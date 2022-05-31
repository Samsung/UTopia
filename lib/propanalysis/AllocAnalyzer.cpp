#include "AllocAnalyzer.h"
#include "ftg/utils/ManualAllocLoader.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;

namespace ftg {

AllocAnalyzer::AllocAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                             std::vector<const llvm::Function *> Funcs,
                             const AllocAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs) {
  if (PreReport)
    Report.set(PreReport->get());

  if (Funcs.size() == 0)
    return;

  const auto *F = Funcs[0];
  if (!F)
    return;

  const auto *M = F->getParent();
  if (!M)
    return;

  updateDefault(*M);
  analyze(Funcs);
}

AllocAnalyzer::AllocAnalyzer(const llvm::Module &M,
                             const AllocAnalysisReport *PreReport)
    : ArgFlowAnalyzer(nullptr, {}) {
  if (PreReport)
    Report.set(PreReport->get());
  updateDefault(M);
}

std::unique_ptr<AnalyzerReport> AllocAnalyzer::getReport() {
  return std::make_unique<AllocAnalysisReport>(Report);
}

const AllocAnalysisReport &AllocAnalyzer::result() const { return Report; }

void AllocAnalyzer::analyzeProperty(llvm::Argument &A) {
  if (updateArgFlow(A))
    return;

  analyzeAlloc(A);
  Report.add(getOrCreateArgFlow(A));
}

void AllocAnalyzer::analyzeAlloc(llvm::Argument &A) {
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

void AllocAnalyzer::handleStackFrame(StackFrame &Frame,
                                     std::stack<StackFrame> &DefUseChains,
                                     std::set<Value *> &VisitedNodes) {
  auto *Def = Frame.Value;

  if (VisitedNodes.find(Def) != VisitedNodes.end())
    return;
  VisitedNodes.insert(Def);

  for (User *U : Def->users()) {
    if (!U)
      continue;
    handleUser(Frame, *U, DefUseChains, VisitedNodes);
  }
}

void AllocAnalyzer::handleUser(StackFrame &Frame, llvm::Value &User,
                               std::stack<StackFrame> &DefUseChains,
                               std::set<llvm::Value *> &VisitedNodes) {
  Value *Def = Frame.Value;
  ArgFlow &DefFlow = Frame.AnalysisResult;
  auto &A = DefFlow.getLLVMArg();

  if (auto *CB = dyn_cast<CallBase>(&User)) {
    if (auto *II = dyn_cast<IntrinsicInst>(CB)) {
      if (II->getIntrinsicID() != llvm::Intrinsic::umul_with_overflow)
        return;
      assert((II->getNumOperands() >= 2) && "Unexpected Program State");

      auto *Op0 = II->getOperand(0);
      auto *Op1 = II->getOperand(1);
      if ((VisitedNodes.find(Op0) == VisitedNodes.end()) &&
          (VisitedNodes.find(Op1) == VisitedNodes.end()))
        return;

      DefUseChains.emplace(&User, DefFlow);
      return;
    }

    if (auto *CF = getCalledFunction(*CB)) {
      auto *BB = CB->getParent();
      assert(BB && "Unexpected Program State");
      if (CF->isDeclaration() && mayThrow(*BB))
        return;
      if (CF == A.getParent())
        return;
      for (auto &Param : CF->args()) {
        auto *CallArg = CB->getArgOperand(Param.getArgNo());
        if (CallArg != Def)
          continue;
        if (!CallArg)
          continue;

        analyze(Param);
        auto &CallAF = getOrCreateArgFlow(Param);
        DefFlow.mergeAlloc(CallAF);

        if (!CallAF.isUsedByRet())
          continue;

        DefUseChains.emplace(&User, DefFlow);
      }
    }

    return;
  }

  if (auto *SI = dyn_cast<StoreInst>(&User)) {
    // if current user is store instruction,
    auto *Op0 = SI->getOperand(0);
    auto *Op1 = SI->getOperand(1);
    assert(Op0 && Op1 && "Unexpected Program State");

    // if storedDef has not been visited yet and is not an argument,
    // go traverse Def-Use Chain.
    if (VisitedNodes.find(Op1) == VisitedNodes.end()) {
      DefUseChains.emplace(Op1, DefFlow);
    }

    if ((VisitedNodes.find(Op0) == VisitedNodes.end()) && !isa<Constant>(Op0) &&
        !isa<GlobalValue>(Op0)) {
      DefUseChains.emplace(Op0, DefFlow);
    }

    Op0 = Op0->stripPointerCasts();
    assert(Op0 && "Unexpected Program State");

    if (auto *CB = dyn_cast<CallBase>(Op0)) {
      if (llvm::Function *CF = getCalledFunction(*CB)) {
        auto *BB = CB->getParent();
        assert(BB && "Unexpected Program State");
        if (CF->isDeclaration() && mayThrow(*BB))
          return;
        if (CF == A.getParent())
          return;
        if (isAllocFunction(*CF))
          DefFlow.Allocation |= Alloc_Address;
      }
      return;
    }

    return;
  }
  if (auto *GEP = dyn_cast<GetElementPtrInst>(&User)) {
    if (GEP->getNumIndices() > 2)
      return;

    // For Array (count of the offset is 1 or baseAddr is Array Type)
    auto *Op0 = GEP->getOperand(0);
    assert(Op0 && "Unexpected Program State");
    if (GEP->getNumIndices() == 1 || getAsArrayType(*Op0)) {
      DefUseChains.emplace(GEP, DefFlow);
      return;
    }

    // Only support for struct GEP, not class/union yet
    auto *ST = getAsStructType(*Op0);
    if (!ST)
      return;

    assert((GEP->getNumOperands() >= GEP->getNumIndices()) &&
           "Unexpected Program State");
    auto *Op = GEP->getOperand(GEP->getNumIndices());
    assert(Op && "Unexpected Program State");

    auto *CI = dyn_cast<ConstantInt>(Op);
    if (!CI)
      return;

    DefFlow.setStruct(ST);

    // offset means field number,
    // create new ArgFlow for field with field number
    auto &FieldFlow = DefFlow.getOrCreateFieldFlow(CI->getLimitedValue());
    FieldFlow.FDInfo->Values.insert(GEP);
    DefUseChains.emplace(GEP, FieldFlow);
    return;
  }

  if (auto *BI = dyn_cast<BitCastInst>(&User)) {
    auto *Op0 = BI->getOperand(0);
    assert(Op0 && "Unexpected Program State");

    auto *FromST = getAsStructType(*Op0);
    auto *ToST = getAsStructType(*BI);

    // not support struct to other, other to struct yet
    if (ToST && !FromST)
      return;

    if (!ToST && FromST) {
      // Follow STptr to nonSTptr casting
      if (!BI->getOperand(0)->getType()->isPointerTy() ||
          !BI->getType()->isPointerTy())
        return;
    }

    if (FromST && ToST) { // struct to struct
      // only support opque(foward declare) to not opaque
      if (!FromST->isOpaque() || ToST->isOpaque())
        return;
      DefFlow.setStruct(ToST);
    }

    DefUseChains.emplace(BI, DefFlow);
    return;
  }

  if (isa<ReturnInst>(&User)) {
    DefFlow.setUsedByRet(true);
    return;
  }

  if (auto *I = dyn_cast<Instruction>(&User)) {
    // if current user is the other instructions,
    if (I->isTerminator() || isa<CmpInst>(I))
      return;

    DefUseChains.emplace(I, DefFlow);
    return;
  }
}

bool AllocAnalyzer::isAllocFunction(const llvm::Function &F) const {
  auto FuncName = F.getName();
  if (FuncName == "malloc" || FuncName == "_Znaj" || FuncName == "_Znam")
    return true;
  return false;
}

bool AllocAnalyzer::updateArgFlow(Argument &A) {
  if (!Report.has(A))
    return false;

  auto &AF = getOrCreateArgFlow(A);
  AF.setArgAlloc(Report.get(A));
  updateFieldFlow(AF);
  return true;
}

void AllocAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  auto *T = A.getType();
  while (isa<llvm::PointerType>(T))
    T = T->getPointerElementType();

  auto *ST = dyn_cast_or_null<llvm::StructType>(T);
  if (!ST)
    return;

  for (unsigned S = 0; S < ST->getNumElements(); ++S) {
    Indices.push_back(S);
    if (Report.has(A, Indices)) {
      AF.setStruct(ST);
      auto &FF = AF.getOrCreateFieldFlow(S);
      FF.setArgAlloc(Report.get(A, Indices));
      updateFieldFlow(FF, Indices);
    }
    Indices.pop_back();
  }
}

void AllocAnalyzer::updateDefault(const llvm::Module &M) {
  ManualAllocLoader MAL;
  for (const auto &F : M) {
    if (!F.isDeclaration())
      continue;

    if (MAL.isAllocFunction(F)) {
      std::map<unsigned, ArgAlloc> DefaultValueMap;
      for (auto AllocSizeArg : MAL.getAllocSizeArgNo(F)) {
        if (AllocSizeArg > F.arg_size())
          continue;
        DefaultValueMap.emplace(AllocSizeArg, Alloc::Alloc_Size);
      }
      updateDefault(F, DefaultValueMap);
      continue;
    }

    if (F.getName() == "free") {
      std::map<unsigned, ArgAlloc> Values = {{0, Alloc::Alloc_Free}};
      updateDefault(F, Values);
      continue;
    }
  }

  for (auto Iter : ArgFlowMap) {
    auto &AF = Iter.second;
    if (!AF)
      continue;

    if (AF->getState() != AnalysisState_Pre_Analyzed)
      continue;

    Report.add(*AF);
  }
}

void AllocAnalyzer::updateDefault(const llvm::Function &F,
                                  const std::map<unsigned, ArgAlloc> &Values) {
  for (const auto &A : F.args()) {
    auto &AF = getOrCreateArgFlow(*const_cast<Argument *>(&A));
    ArgAlloc Alloc = Alloc_None;
    auto Iter = Values.find(A.getArgNo());
    if (Iter != Values.end())
      Alloc = Iter->second;
    AF.setArgAlloc(Alloc);
    AF.setState(AnalysisState_Pre_Analyzed);
  }
}

} // namespace ftg
