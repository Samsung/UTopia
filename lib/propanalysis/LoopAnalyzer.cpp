#include "ftg/propanalysis/LoopAnalyzer.h"
#include "ftg/propanalysis/PropAnalysisCommon.h"
#include "ftg/utils/LLVMUtil.h"

using namespace llvm;

namespace ftg {

LoopAnalyzer::LoopAnalyzer(IndCallSolverMgr *Solver,
                           std::vector<const llvm::Function *> Funcs,
                           llvm::FunctionAnalysisManager &FAM,
                           const LoopAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs), FAM(FAM) {
  if (PreReport) {
    for (const auto &Iter : PreReport->get())
      Report.set(Iter.first, Iter.second);
  }
  analyze(Funcs);
}

std::unique_ptr<AnalyzerReport> LoopAnalyzer::getReport() {
  return std::make_unique<LoopAnalysisReport>(Report);
}

const LoopAnalysisReport &LoopAnalyzer::result() const { return Report; }

void LoopAnalyzer::analyzeProperty(llvm::Argument &A) {
  if (updateArgFlow(A))
    return;

  analyzeLoop(A);
  Report.add(getOrCreateArgFlow(A));
}

void LoopAnalyzer::analyzeLoop(llvm::Argument &A) {
  std::set<Value *> VisitedNodes;

  ArgFlow &AF = getOrCreateArgFlow(A);

  // DefUseChanins used as stack,
  // StackFrame includes the value(Argument/Field) need to tracking,
  // and ArgFlow for which analysis result will be updated.
  std::stack<StackFrame> DefUseChains;
  DefUseChains.emplace(&A, AF);

  while (!DefUseChains.empty()) {
    auto Frame = DefUseChains.top();
    DefUseChains.pop();
    handleStackFrame(Frame, DefUseChains, VisitedNodes);
  }
}

void LoopAnalyzer::handleStackFrame(StackFrame &Frame,
                                    std::stack<StackFrame> &DefUseChains,
                                    std::set<Value *> &VisitedNodes) {
  auto *Def = Frame.Value;
  assert(Def && "Unexpected Program State");

  if (VisitedNodes.find(Def) != VisitedNodes.end() ||
      !isa<IntegerType>(Def->getType()))
    return;
  VisitedNodes.insert(Def);

  for (User *U : Def->users()) {
    if (!U)
      continue;

    if (handleUser(Frame, *U, DefUseChains, VisitedNodes))
      break;
  }
}

bool LoopAnalyzer::handleUser(StackFrame &Frame, llvm::Value &User,
                              std::stack<StackFrame> &DefUseChains,
                              std::set<llvm::Value *> &VisitedNodes) {
  Value *Def = Frame.Value;
  ArgFlow &DefFlow = Frame.AnalysisResult;
  auto &A = DefFlow.getLLVMArg();

  if (!isa<Instruction>(&User))
    return false;

  if (auto *ICI = dyn_cast<ICmpInst>(&User)) {
    auto *F = A.getParent();
    auto *BB = ICI->getParent();
    if (!F || !BB)
      return false;

    auto &LI = FAM.getResult<LoopAnalysis>(*F);
    auto *L = LI.getLoopFor(BB);
    if (!L)
      return false;

    if (isUsedAsInitializer(*ICI, *L, VisitedNodes))
      return false;

    auto *ExitCond = getLoopExitCond(*L);
    if (!ExitCond || ExitCond != ICI)
      return false;

    llvm::Value *Op = ExitCond->getOperand(0);
    if (Op != Def)
      Op = ExitCond->getOperand(1);
    if (Op != Def)
      return false;
    if (!L->isLoopInvariant(Op)) {
      return false;
    }

    DefFlow.LoopExit = true;
    DefFlow.LoopDepth = LI.getLoopDepth(BB);
    return true;
  }

  if (auto *CB = dyn_cast<CallBase>(&User)) {
    auto *CF = util::getCalledFunction(*CB, Solver);
    if (!CF)
      return false;
    if (CF == A.getParent())
      return false;

    for (auto &Param : CF->args()) {
      auto *CallArg = CB->getArgOperand(Param.getArgNo());
      if (CallArg != Def)
        continue;
      if (!CallArg)
        continue;

      analyze(Param);
      auto &CallAF = getOrCreateArgFlow(Param);
      DefFlow.LoopExit = CallAF.LoopExit;
      DefFlow.LoopDepth = CallAF.LoopDepth;
      if (DefFlow.LoopExit)
        return true;
    }
    return false;
  }

  if (auto *SI = dyn_cast<StoreInst>(&User)) {
    Value *V;
    if (SI->getOperand(0) == Def)
      V = SI->getOperand(1);
    else
      V = SI->getOperand(0);

    DefUseChains.emplace(V, DefFlow);
    return false;
  }

  if (auto *CI = dyn_cast<CastInst>(&User)) {
    if (isa<llvm::IntegerType>(CI->getType()))
      DefUseChains.emplace(CI, DefFlow);
    return false;
  }

  if (isa<BinaryOperator>(&User) || isa<LoadInst>(&User)) {
    if (isa<llvm::IntegerType>(User.getType()))
      DefUseChains.emplace(&User, DefFlow);
    return false;
  }

  if (isa<PHINode>(&User) || isa<SelectInst>(&User)) {
    DefUseChains.emplace(&User, DefFlow);
    return false;
  }

  return DefFlow.LoopExit;
}

bool LoopAnalyzer::isUsedAsInitializer(
    const ICmpInst &I, const Loop &L,
    const std::set<llvm::Value *> &VisitedNodes) {
  for (const auto &Operand : I.operands()) {
    const auto *PN = dyn_cast<PHINode>(Operand);
    if (!PN)
      continue;

    for (const auto &B : PN->blocks()) {
      if (!B)
        continue;

      if (L.contains(B))
        continue;

      auto *V = PN->getIncomingValueForBlock(B);
      if (!V)
        continue;

      if (VisitedNodes.find(V) == VisitedNodes.end())
        continue;

      return true;
    }
  }
  return false;
}

bool LoopAnalyzer::updateArgFlow(Argument &A) {
  if (!Report.has(A))
    return false;

  auto &AF = getOrCreateArgFlow(A);
  auto Value = Report.get(A);
  AF.LoopExit = Value.LoopExit;
  AF.LoopDepth = Value.LoopDepth;
  updateFieldFlow(AF);
  return true;
}

void LoopAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  auto *T = A.getType();
  while (isa<llvm::PointerType>(T) && T->getNumContainedTypes() > 0) {
    T = T->getContainedType(0);
  }

  auto *ST = dyn_cast_or_null<llvm::StructType>(T);
  if (!ST)
    return;

  for (unsigned S = 0; S < ST->getNumElements(); ++S) {
    Indices.push_back(S);
    if (!Report.has(A, Indices)) {
      Indices.pop_back();
      continue;
    }

    AF.setStruct(ST);
    auto &FF = AF.getOrCreateFieldFlow(S);
    auto Value = Report.get(A, Indices);
    FF.LoopExit = Value.LoopExit;
    FF.LoopDepth = Value.LoopDepth;
    auto FI = FF.getFieldInfo();
    updateFieldFlow(FF, Indices);
    Indices.pop_back();
  }
}

} // namespace ftg
