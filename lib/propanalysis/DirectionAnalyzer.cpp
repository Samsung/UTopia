#include "DirectionAnalyzer.h"
#include "ftg/utils/LLVMUtil.h"
#include "ftg/utils/ManualAllocLoader.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Operator.h"
#include <llvm/IR/Module.h>

using namespace llvm;

namespace ftg {

DirectionAnalyzer::DirectionAnalyzer(
    IndCallSolverMgr *Solver, const std::vector<const llvm::Function *> &Funcs,
    const DirectionAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs),
      Report(std::make_unique<DirectionAnalysisReport>()) {
  if (PreReport) {
    for (const auto &Iter : PreReport->get())
      Report->set(Iter.first, Iter.second);
  }

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

std::unique_ptr<AnalyzerReport> DirectionAnalyzer::getReport() {
  return std::move(Report);
}

const DirectionAnalysisReport &DirectionAnalyzer::result() const {
  assert(Report && "Unexpected Program State");
  return *Report;
}

bool DirectionAnalyzer::isPtrToVal(const llvm::Value &V) const {
  auto *T = V.getType();
  return T && T->isPointerTy() && T->getNumContainedTypes() > 0 &&
         !(T->getContainedType(0)->isPointerTy());
}

void DirectionAnalyzer::analyzeProperty(llvm::Argument &A) {
  if (updateArgFlow(A))
    return;

  analyzeDirection(A);
  assert(Report && "Unexpected Program State");
  Report->add(getOrCreateArgFlow(A));
}

void DirectionAnalyzer::analyzeDirection(llvm::Argument &A) {

  std::set<Value *> VisitedNodes;

  ArgFlow &AF = getOrCreateArgFlow(A);

  if (auto *T = A.getType()) {
    if (!T->isPointerTy()) {
      AF |= Dir_In;
      return;
    }
  }

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

void DirectionAnalyzer::handleStackFrame(StackFrame &Frame,
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

void DirectionAnalyzer::handleUser(StackFrame &Frame, llvm::Value &User,
                                   std::stack<StackFrame> &DefUseChains,
                                   std::set<llvm::Value *> &VisitedNodes) {
  Value *Def = Frame.Value;
  ArgFlow &DefFlow = Frame.AnalysisResult;
  auto &A = DefFlow.getLLVMArg();

  if (auto *CB = dyn_cast<CallBase>(&User)) {
    auto *CF = util::getCalledFunction(*CB, Solver);
    if (!CF) {
      DefFlow |= Dir_Unidentified;
      return;
    }

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
      DefFlow.mergeDirection(CallAF);
    }
    return;
  }

  if (auto *SI = dyn_cast<StoreInst>(&User)) {
    // if current user is store instruction,
    auto *Op1 = SI->getOperand(1);
    assert(Op1 && "Unexpected Program State");

    // if storedDef has not been visited yet and is not an argument,
    // go traverse Def-Use Chain.
    if (VisitedNodes.find(Op1) != VisitedNodes.end()) {
      if (isa<GetElementPtrInst>(Op1))
        DefFlow |= Dir_In;

      auto *ValueOp = SI->getValueOperand();
      if (VisitedNodes.find(ValueOp) == VisitedNodes.end()) {
        DefFlow |= Dir_Out;
      }
    }

    if ((VisitedNodes.find(Op1) == VisitedNodes.end()) && !isa<Argument>(Op1))
      DefUseChains.emplace(Op1, DefFlow);

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
    FieldFlow.getFieldInfo()->Values.insert(GEP);
    DefUseChains.emplace(GEP, FieldFlow);
    return;
  }

  if (auto *BI = dyn_cast<BitCastInst>(&User)) {
    auto *Op0 = BI->getOperand(0);
    assert(Op0 && "Unexpected Program State");

    auto *FromST = getAsStructType(*Op0);
    auto *ToST = getAsStructType(*BI);

    // not support struct to other, other to struct yet
    if ((!ToST && FromST) || (ToST && !FromST))
      return;

    if (FromST && ToST) { // struct to struct
      // only support opque(foward declare) to not opaque
      if (!FromST->isOpaque() || ToST->isOpaque())
        return;
      DefFlow.setStruct(ToST);
    }

    DefUseChains.emplace(BI, DefFlow);
    return;
  }

  if (auto *I = dyn_cast<Instruction>(&User)) {
    // if current user is the other instructions,
    if (I->isTerminator() || isa<CmpInst>(I))
      return;

    if (I->mayReadFromMemory() || isa<Operator>(I)) {
      if (auto *LI = dyn_cast<LoadInst>(I)) {
        auto *Op0 = LI->getOperand(0);
        assert(Op0 && "Unexpected Program State");

        if ((VisitedNodes.find(Op0) != VisitedNodes.end()) &&
            isPtrToVal(*Op0)) {
          DefFlow |= Dir_In;
        }
      }

      DefUseChains.emplace(I, DefFlow);
      return;
    }

    DefFlow |= Dir_Unidentified;
    if (I->mayWriteToMemory())
      return;

    DefUseChains.emplace(I, DefFlow);
    return;
  }
}

bool DirectionAnalyzer::updateArgFlow(Argument &A) {
  if (!Report->has(A))
    return false;

  auto &AF = getOrCreateArgFlow(A);
  AF.setArgDir(Report->get(A));
  updateFieldFlow(AF);
  return true;
}

void DirectionAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
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
    if (Report->has(A, Indices)) {
      AF.setStruct(ST);
      auto &FF = AF.getOrCreateFieldFlow(S);
      FF.setArgDir(Report->get(A, Indices));
      updateFieldFlow(FF, Indices);
    }
    Indices.pop_back();
  }
}

void DirectionAnalyzer::updateDefault(const llvm::Module &M) {
  if (!Report)
    return;

  const std::map<Intrinsic::ID, std::map<unsigned, ArgDir>>
      DefaultIntrinsicMap = {
          {{Intrinsic::memcpy, {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
           {Intrinsic::memmove, {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
           {Intrinsic::memset, {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
           {Intrinsic::memcpy_element_unordered_atomic,
            {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
           {Intrinsic::memmove_element_unordered_atomic,
            {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
           {Intrinsic::memset_element_unordered_atomic,
            {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}}}};
  const std::map<std::string, std::map<unsigned, ArgDir>> DefaultNameMap = {
      {{"memcpy", {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
       {"memmove", {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
       {"memset", {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
       {"memcpy_element_unordered_atomic",
        {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
       {"memmove_element_unordered_atomic",
        {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}},
       {"memset_element_unordered_atomic",
        {{0, Dir_Out}, {1, Dir_In}, {2, Dir_In}}}}};

  ManualAllocLoader MAL;
  for (const auto &F : M) {
    if (!F.isDeclaration())
      continue;

    auto IterIntrinsic = DefaultIntrinsicMap.find(F.getIntrinsicID());
    if (IterIntrinsic != DefaultIntrinsicMap.end())
      updateDefault(F, IterIntrinsic->second);

    auto IterName = DefaultNameMap.find(F.getName().str());
    if (IterName != DefaultNameMap.end())
      updateDefault(F, IterName->second);

    if (MAL.isAllocFunction(F)) {
      for (auto AllocSizeArg : MAL.getAllocSizeArgNo(F)) {
        if (AllocSizeArg > F.arg_size())
          continue;

        std::map<unsigned, ArgDir> Values = {{AllocSizeArg, Dir_In}};
        updateDefault(F, Values);
      }
      continue;
    }

    if (F.getName() == "free") {
      std::map<unsigned, ArgDir> Values = {{0, Dir_In}};
      updateDefault(F, Values);
    }
  }

  for (auto Iter : ArgFlowMap) {
    auto &AF = Iter.second;
    if (!AF)
      continue;

    if (AF->getState() != AnalysisState_Pre_Analyzed)
      continue;

    Report->add(*AF);
  }
}

void DirectionAnalyzer::updateDefault(
    const llvm::Function &F, const std::map<unsigned, ArgDir> &Values) {
  for (const auto &A : F.args()) {
    auto &AF = getOrCreateArgFlow(*const_cast<Argument *>(&A));
    ArgDir Dir = Dir_Unidentified;
    auto Iter = Values.find(A.getArgNo());
    if (Iter != Values.end())
      Dir = Iter->second;
    AF.setArgDir(Dir);
    AF.setState(AnalysisState_Pre_Analyzed);
  }
}

} // namespace ftg
