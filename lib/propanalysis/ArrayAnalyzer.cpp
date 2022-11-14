#include "ftg/propanalysis/ArrayAnalyzer.h"
#include "ftg/propanalysis/PropAnalysisCommon.h"
#include "ftg/utils/LLVMUtil.h"
#include "llvm/Analysis/IVDescriptors.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace llvm;

namespace ftg {

ArrayAnalyzer::ArrayAnalyzer(IndCallSolverMgr *Solver,
                             std::vector<const llvm::Function *> Funcs,
                             llvm::FunctionAnalysisManager &FAM,
                             const ArrayAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs), FAM(FAM),
      Report(std::make_unique<ArrayAnalysisReport>()) {
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

std::unique_ptr<AnalyzerReport> ArrayAnalyzer::getReport() {
  return std::move(Report);
}

const ArrayAnalysisReport &ArrayAnalyzer::result() const {
  assert(Report && "Unexpected Program State");
  return *Report;
}

void ArrayAnalyzer::analyzeProperty(llvm::Argument &A) {
  if (updateArgFlow(A))
    return;

  analyzeArray(A);
  assert(Report && "Unexpected Program State");
  Report->add(getOrCreateArgFlow(A));
}

void ArrayAnalyzer::analyzeArray(llvm::Argument &A) {
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

void ArrayAnalyzer::analyzeArrayLen(Instruction &I, ArgFlow &AF) {
  auto &LA = FAM.getResult<LoopAnalysis>(*AF.getLLVMArg().getParent());
  Value *TrackingV = nullptr;
  assert(I.getNumOperands() > 1 && "Unexpected Program State");

  if (auto *L = LA.getLoopFor(I.getParent())) {
    // Array ptr/index is invariant in the loop
    if (L->isLoopInvariant(I.getOperand(0)) &&
        L->isLoopInvariant(I.getOperand(1))) {
      TrackingV = I.getOperand(1);
    } else {
      // Get the variable need to check
      // if the count of iterations in the loop is dependent on the
      // Argument/Field
      TrackingV = getTrackingVariable(*L);
    }
  } else {
    TrackingV = I.getOperand(1);
  }
  if (!TrackingV)
    return;

  // Check variable has dependency with Argument/Field
  if (AF.isField())
    AF.collectRelatedLengthField(*TrackingV);
  else
    AF.collectRelatedLengthArg(*TrackingV);

  // if SizeFields has elements,
  // it's means ArrayLen is realted to Argument/Field,
  // get a ArgFlow for the Argument/Field, then set to ArrayLen
  if (AF.isField()) { // For Field
    auto FI = AF.getFieldInfo();
    assert(FI && "Unexpected Program State");

    auto &ParentFlow = FI->Parent;
    for (auto FieldNum : FI->SizeFields) {
      auto &FiedFlow = ParentFlow.getOrCreateFieldFlow(FieldNum);
      FiedFlow.setToArrLen(FI->FieldNum);
    }
  } else { // For Argument
    for (auto SizeArg : AF.getSizeArgs()) {
      auto &A = AF.getLLVMArg();
      if (SizeArg->getParent() != A.getParent())
        continue;

      analyze(*SizeArg);
      auto &LenFlow = getOrCreateArgFlow(*SizeArg);
      LenFlow.setToArrLen(A);
    }
  }
}

// This API returns a variable would be tracked
// TODO:
// This API analyze single loop, not a nested loop.
// If there is an instruction (array access) in the outer loop,
// only the outer loop is analyzed
// If exist in inner loop, analyze only inner loop
// Nested loop support is required when analyzing ranges of array index.
Value *ArrayAnalyzer::getTrackingVariable(Loop &L) {
  bool PositiveStride = false;

  auto LoopHeader = L.getHeader();
  assert(LoopHeader && "Unexpected Program State");

  auto *F = LoopHeader->getParent();
  assert(F && "Unexpected Program State");

  auto &SEA = FAM.getResult<ScalarEvolutionAnalysis>(*F);
  auto *VariantV = getRelatedIndV(L, SEA);
  auto *ExitCond = getLoopExitCond(L);
  if (!ExitCond)
    return nullptr;
  assert(ExitCond->getNumOperands() > 1 && "Unexpected Program State");

  auto *CmpOp0 = ExitCond->getOperand(0);
  auto *CmpOp1 = ExitCond->getOperand(1);

  // For a variable dependent on the induction variable,
  // use the Scalar Evolution to get a phinode(loop-variant) and step
  if (VariantV) {
    if (const auto *AR = dyn_cast<SCEVAddRecExpr>(SEA.getSCEV(VariantV))) {
      const auto *Stride = AR->getStepRecurrence(SEA);
      PositiveStride = SEA.isKnownPositive(Stride);
    }
  } else {
    // For non induction variable, analyze loop termination
    // to get the phinode(loop-variant) and step
    //
    // ex) while(size > 0) size -= getSize()
    //
    // The induction variable increases/decreases by a fixed amount.
    // However the "size" is reduced by the external API's return value.
    // So "size" is not induction variable.
    // There is no induction variable in this loop
    for (unsigned CmpOpNum = 0, E1 = ExitCond->getNumOperands(); CmpOpNum < E1;
         CmpOpNum++) {
      auto *PN = dyn_cast<PHINode>(ExitCond->getOperand(CmpOpNum));
      if (!PN)
        continue;

      VariantV = ExitCond->getOperand(CmpOpNum);

      for (unsigned PnOpNum = 0, E2 = PN->getNumIncomingValues(); PnOpNum < E2;
           PnOpNum++) {
        if (!L.contains(PN->getIncomingBlock(PnOpNum)))
          continue;

        auto *BO = dyn_cast<BinaryOperator>(PN->getIncomingValue(PnOpNum));
        if (!BO)
          continue;

        if (BO->getOpcode() == Instruction::Add ||
            BO->getOpcode() == Instruction::Mul ||
            BO->getOpcode() == Instruction::FAdd ||
            BO->getOpcode() == Instruction::FMul)
          PositiveStride = true;
      }
      break;
    }
  }

  if (!VariantV)
    return nullptr;

  // loop's iteration step is negative, track the variant value
  // case : for(i = len; i > 0; i--) <- track "i"
  if (!PositiveStride)
    return VariantV;

  // loop's iteration step is positive, track the operand of exit condition
  // case : for(i = 0; i < len; i++) <- track "len"(operand 1)
  if (VariantV == CmpOp0)
    return CmpOp1;

  // case : for(i = 0; len > i; i++) <- track "len"(operand 0)
  if (VariantV == CmpOp1)
    return CmpOp0;

  return nullptr;
}

// This API returns an induction variable or
// A variable dependent on the induction variable
Value *ArrayAnalyzer::getRelatedIndV(Loop &L, ScalarEvolution &SEA) {
  if (!L.isLoopSimplifyForm())
    return nullptr;

  auto *Header = L.getHeader();
  assert(Header && "Unexpected Program State");

  ICmpInst *CmpInst = getLoopExitCond(L);
  if (!CmpInst)
    return nullptr;
  assert(CmpInst->getNumOperands() > 1 && "Unexpected Program State");

  auto *LatchCmpOp0 = dyn_cast<Instruction>(CmpInst->getOperand(0));
  auto *LatchCmpOp1 = dyn_cast<Instruction>(CmpInst->getOperand(1));

  for (auto &IndVar : Header->phis()) {
    InductionDescriptor IndDesc;
    if (!InductionDescriptor::isInductionPHI(&IndVar, &L, &SEA, IndDesc))
      continue;

    // case 1: Induction Variable
    if (&IndVar == LatchCmpOp0 || &IndVar == LatchCmpOp1)
      return &IndVar;

    // case 2: If IndVar is related to the operands of ICMP inst,
    // it also depend on the count of loop
    // ex) for (i = 0; i+1 < len; i++) : "i+1" dependent on IndVar
    if (LatchCmpOp0) {
      for (unsigned Opnum = 0; Opnum < LatchCmpOp0->getNumOperands(); Opnum++) {
        if (LatchCmpOp0->getOperand(Opnum) != &IndVar)
          continue;
        return LatchCmpOp0;
      }
    }

    // ex) for (i = 0; len > i+1; i++)
    if (LatchCmpOp1) {
      for (unsigned Opnum = 0; Opnum < LatchCmpOp1->getNumOperands(); Opnum++) {
        if (LatchCmpOp1->getOperand(Opnum) != &IndVar)
          continue;
        return LatchCmpOp1;
      }
    }
  }

  return nullptr;
}

void ArrayAnalyzer::handleStackFrame(StackFrame &Frame,
                                     std::stack<StackFrame> &DefUseChains,
                                     std::set<Value *> &VisitedNodes) {
  auto *Def = Frame.Value;
  assert(Def && "Unexpected Program State");

  if (VisitedNodes.find(Def) != VisitedNodes.end())
    return;
  if (isa<IntegerType>(Def->getType()))
    return;
  VisitedNodes.insert(Def);

  for (User *U : Def->users()) {
    if (!U)
      continue;
    handleUser(Frame, *U, DefUseChains, VisitedNodes);
  }
}

void ArrayAnalyzer::handleUser(StackFrame &Frame, llvm::Value &User,
                               std::stack<StackFrame> &DefUseChains,
                               std::set<llvm::Value *> &VisitedNodes) {
  Value *Def = Frame.Value;
  ArgFlow &DefFlow = Frame.AnalysisResult;
  auto &A = DefFlow.getLLVMArg();

  if (auto *GEPI = dyn_cast<GetElementPtrInst>(&User)) {
    if (GEPI->getNumIndices() > 2)
      return;

    auto *Op0 = GEPI->getOperand(0);
    assert(Op0 && "Unexpected Program State");
    auto *ST = getAsStructType(*Op0);
    auto NumIndices = GEPI->getNumIndices();

    if (NumIndices > 1 && !getAsArrayType(*Op0)) {
      if (ST) {
        assert((GEPI->getNumOperands() > NumIndices) &&
               "Unexpected Program State");

        if (const auto *CI =
                dyn_cast<ConstantInt>(GEPI->getOperand(NumIndices))) {
          DefFlow.setStruct(ST);
          auto &FieldFlow = DefFlow.getOrCreateFieldFlow(CI->getLimitedValue());
          FieldFlow.getFieldInfo()->Values.insert(GEPI);
          DefUseChains.emplace(GEPI, FieldFlow);
          return;
        }
      }
    } else if (ST) { // for Struct Array
      DefUseChains.emplace(GEPI, DefFlow);
    }

    if (VisitedNodes.find(Op0) != VisitedNodes.end()) {
      assert((GEPI->getNumOperands() > NumIndices) &&
             "Unexpected Program State");

      if (const auto *CI =
              dyn_cast<ConstantInt>(GEPI->getOperand(NumIndices))) {
        if (CI->isZeroValue()) {
          DefUseChains.emplace(GEPI, DefFlow);
          return;
        }

        DefFlow.getArrIndices().insert(CI->getZExtValue());

        auto &LI = FAM.getResult<LoopAnalysis>(*A.getParent());
        if (LI.getLoopFor(GEPI->getParent()))
          DefFlow.setIsVariableLenArray(true);
      } else
        DefFlow.setIsVariableLenArray(true);

      if (DefFlow.isVariableLenArray())
        analyzeArrayLen(*GEPI, DefFlow);

      DefFlow.setIsArray(true);
    }
    return;
  }

  if (auto *SI = dyn_cast<StoreInst>(&User)) {
    auto *Op0 = SI->getOperand(0);
    auto *Op1 = SI->getOperand(1);

    if (VisitedNodes.find(Op1) == VisitedNodes.end()) {
      DefUseChains.emplace(Op1, DefFlow);
    }

    if ((VisitedNodes.find(Op0) == VisitedNodes.end()) && !isa<Constant>(Op0) &&
        !isa<GlobalValue>(Op0)) {
      DefUseChains.emplace(Op0, DefFlow);
    }
    return;
  }

  if (auto *CB = dyn_cast<CallBase>(&User)) {
    auto *CF = util::getCalledFunction(*CB, Solver);
    if (!CF)
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
      DefFlow.mergeArray(CallAF, *CB);
    }
    return;
  }

  if (auto *BI = dyn_cast<BitCastInst>(&User)) {
    if (isUnionFieldAccess(BI))
      return;

    auto *Op0 = BI->getOperand(0);
    assert(Op0 && "Unexpected Program State");
    auto *FromST = getAsStructType(*Op0);
    auto *ToST = getAsStructType(*BI);

    // not support struct to other, other to struct yet
    if ((!ToST && FromST) || (ToST && !FromST))
      return;

    if (FromST && ToST) { // struct to struct
      // only support opque(foward declare) to not opaque
      if (FromST->isOpaque() && !ToST->isOpaque())
        DefFlow.setStruct(ToST);
      else
        return;
    }
    DefUseChains.emplace(BI, DefFlow);
    return;
  }

  if (auto *I = dyn_cast<Instruction>(&User)) {
    if (I->isTerminator() || isa<CmpInst>(I))
      return;
    DefUseChains.emplace(&User, DefFlow);
  }
}

bool ArrayAnalyzer::updateArgFlow(Argument &A) {
  if (!Report->has(A))
    return false;

  auto &AF = getOrCreateArgFlow(A);

  auto Value = Report->get(A);
  if (Value == ArrayAnalysisReport::NO_ARRAY)
    AF.setIsArray(false);
  else
    AF.setIsArray(true);

  if (Value >= 0) {
    auto *F = AF.getLLVMArg().getParent();
    if (!F)
      return true;

    auto *ResultArg = F->getArg(Report->get(A));
    if (!ResultArg)
      return true;

    AF.setSizeArg(*ResultArg);
  }
  updateFieldFlow(AF);
  return true;
}

void ArrayAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  auto *T = A.getType();
  while (isa<llvm::PointerType>(T))
    T = T->getPointerElementType();

  auto *ST = dyn_cast_or_null<llvm::StructType>(T);
  if (!ST)
    return;

  for (unsigned S = 0; S < ST->getNumElements(); ++S) {
    Indices.push_back(S);
    if (!Report->has(A, Indices)) {
      Indices.pop_back();
      continue;
    }

    AF.setStruct(ST);
    auto &FF = AF.getOrCreateFieldFlow(S);
    auto Value = Report->get(A, Indices);
    if (Value == ArrayAnalysisReport::NO_ARRAY)
      FF.setIsArray(false);
    else
      FF.setIsArray(true);

    auto FI = FF.getFieldInfo();
    if (Value >= 0 && FI)
      FI->SizeFields = {(unsigned)Value};
    updateFieldFlow(FF, Indices);
    Indices.pop_back();
  }
}

void ArrayAnalyzer::updateDefault(const llvm::Module &M) {
  if (!Report)
    return;

  const std::map<Intrinsic::ID, std::map<unsigned, int>> DefaultIntrinsicMap = {
      {{Intrinsic::memcpy, {{0, 2}, {1, 2}}},
       {Intrinsic::memmove, {{0, 2}, {1, 2}}},
       {Intrinsic::memset, {{0, 2}, {1, 2}}},
       {Intrinsic::memcpy_element_unordered_atomic, {{0, 2}, {1, 2}}},
       {Intrinsic::memmove_element_unordered_atomic, {{0, 2}, {1, 2}}},
       {Intrinsic::memset_element_unordered_atomic, {{0, 2}, {1, 2}}}}};
  const std::map<std::string, std::map<unsigned, int>> DefaultNameMap = {
      {{"memcpy", {{0, 2}, {1, 2}}},
       {"memmove", {{0, 2}, {1, 2}}},
       {"memset", {{0, 2}, {1, 2}}},
       {"memcpy_element_unordered_atomic", {{0, 2}, {1, 2}}},
       {"memmove_element_unordered_atomic", {{0, 2}, {1, 2}}},
       {"memset_element_unordered_atomic", {{0, 2}, {1, 2}}}}};

  for (const auto &F : M) {
    if (!F.isDeclaration())
      continue;

    auto IterIntrinsic = DefaultIntrinsicMap.find(F.getIntrinsicID());
    if (IterIntrinsic != DefaultIntrinsicMap.end()) {
      updateDefault(F, IterIntrinsic->second);
      continue;
    }

    auto IterName = DefaultNameMap.find(F.getName().str());
    if (IterName != DefaultNameMap.end())
      updateDefault(F, IterName->second);
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

void ArrayAnalyzer::updateDefault(const llvm::Function &F,
                                  const std::map<unsigned, int> &Values) {
  for (const auto &Arg : F.args()) {
    auto Iter = Values.find(Arg.getArgNo());
    int ArgLenIdx = ArrayAnalysisReport::NO_ARRAY;
    if (Iter != Values.end())
      ArgLenIdx = Iter->second;
    if (ArgLenIdx >= 0 && static_cast<unsigned>(ArgLenIdx) > F.arg_size())
      ArgLenIdx = ArrayAnalysisReport::ARRAY_NOLEN;

    auto &AF = getOrCreateArgFlow(*const_cast<Argument *>(&Arg));
    AF.setState(AnalysisState_Pre_Analyzed);
    if (ArgLenIdx == ArrayAnalysisReport::NO_ARRAY) {
      AF.setIsArray(false);
      continue;
    }

    AF.setIsArray(true);
    if (ArgLenIdx < 0)
      continue;

    auto *ArgLen = const_cast<Argument *>(F.getArg(ArgLenIdx));
    if (!ArgLen)
      continue;

    AF.collectRelatedLengthArg(*ArgLen);
  }
}

} // namespace ftg
