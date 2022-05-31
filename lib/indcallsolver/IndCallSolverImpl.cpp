#include "ftg/indcallsolver/IndCallSolverImpl.h"
#include "ftg/indcallsolver/CustomCVPPass.h"
#include "llvm/Analysis/TypeMetadataUtils.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

namespace ftg {

struct TypeMetaInfo {
  // GlobalVariable what has type metadata
  GlobalVariable *GV;
  // Offset at initializer of GlobalVariable
  uint64_t GVOffset;

  bool operator<(const TypeMetaInfo &Other) const {
    return GV < Other.GV || (GV == Other.GV && GVOffset < Other.GVOffset);
  }
};

void IndCallSolverImpl::solve(Module &M) {
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  // add customized CVP pass
  MPM.addPass(CustomCVPPass());
  MPM.run(M, MAM);
  buildVTMap(M);     // build map with type metadata and llvm.type.test info
  buildGlobalMap(M); // build map with global variable
  buildInstMap(M);   // build map with tbaa/instruction
}

void IndCallSolverImpl::buildGlobalMap(llvm::Module &M) {
  for (auto &GV : M.globals()) {
    if (!GV.hasInitializer())
      continue;

    auto *Init = GV.getInitializer();
    if (!Init)
      continue;

    auto *STy = dyn_cast<StructType>(Init->getType());
    if (!STy) {
      if (auto *F = dyn_cast<Function>(Init))
        GlobalMap[GV.getType()][0].insert(F);
      continue;
    }

    for (unsigned S = 0, E = STy->getNumElements(); S < E; ++S) {
      if (auto *F = dyn_cast<Function>(Init->getAggregateElement(S)))
        GlobalMap[GV.getType()][S].insert(F);
    }
  }
}

void IndCallSolverImpl::buildInstMap(llvm::Module &M) {
  for (auto &F : M.getFunctionList()) {
    for (auto &BB : F.getBasicBlockList()) {
      for (auto &I : BB.getInstList()) {
        if (!isa<StoreInst>(I))
          continue;

        // if pointer to the function and tbaa MD exists, add to InstMap
        auto *Node = I.getMetadata(LLVMContext::MD_tbaa);
        if (!Node)
          continue;

        auto *F = dyn_cast<Function>(I.getOperand(0));
        if (!F)
          continue;

        InstMap[Node][F->getType()].insert(F);
      }
    }
  }
}

void IndCallSolverImpl::buildVTMap(llvm::Module &M) {
  DenseMap<Metadata *, std::set<TypeMetaInfo>> TypeIdMap;
  SmallVector<MDNode *, 2> Types;
  auto *TypeTestFunc = Intrinsic::getDeclaration(&M, Intrinsic::type_test);

  // Step 1. Build TypeID Map with type metadata
  for (auto &GV : M.globals()) {
    Types.clear();
    GV.getMetadata(LLVMContext::MD_type, Types);
    if (GV.isDeclaration() || Types.empty())
      continue;

    for (auto *Type : Types) {
      if (!Type)
        continue;

      auto *TypeID = Type->getOperand(1).get();
      auto *CM = dyn_cast<ConstantAsMetadata>(Type->getOperand(0));
      if (!CM)
        continue;

      auto *V = CM->getValue();
      if (!V)
        continue;

      auto *CI = dyn_cast<ConstantInt>(V);
      if (!CI)
        continue;

      auto Offset = CI->getZExtValue();
      TypeIdMap[TypeID].insert({&GV, Offset});
    }
  }

  for (auto &U : TypeTestFunc->uses()) {
    auto *CI = dyn_cast<CallInst>(U.getUser());
    if (!CI)
      continue;

    // Step 2. findDevirtCalls using llvm.type.test instruction
    SmallVector<DevirtCallSite, 1> DevirtCalls;
    auto *Op = CI->getArgOperand(0);
    if (!Op)
      continue;

    Op = Op->stripPointerCasts();
    if (!Op)
      continue;

    findDeVirtualizableCalls(DevirtCalls, *Op, 0, M);

    // Step 3. build VTMap with TypeID/DevirtCalls
    for (auto &Call : DevirtCalls) {
      auto *Arg1 = CI->getArgOperand(1);
      if (!Arg1)
        continue;

      auto *MAV = dyn_cast<MetadataAsValue>(Arg1);
      if (!MAV)
        continue;

      for (const TypeMetaInfo TM : TypeIdMap[MAV->getMetadata()]) {
        if (!TM.GV->isConstant())
          continue;

        auto *Ptr = llvm::getPointerAtOffset(TM.GV->getInitializer(),
                                             TM.GVOffset + Call.VTOffset, M);
        if (!Ptr)
          continue;

        Ptr = Ptr->stripPointerCasts();
        if (!Ptr)
          continue;

        auto *F = dyn_cast<Function>(Ptr);
        if (!F)
          continue;

        VTMap[Call.CV].insert(F);
      }
    }
  }
}

llvm::Function *IndCallSolverImpl::getCalledFunction(CallBase &CB) {
  // Step 1. check strip Pointer
  auto *CV = CB.getCalledValue();
  if (!CV)
    return nullptr;

  auto *V = CV->stripPointerCasts();
  if (!V)
    return nullptr;

  if (V->getName() != "") {
    if (auto *F = dyn_cast<Function>(V))
      return F;

    if (auto *GA = dyn_cast<GlobalAlias>(V))
      return dyn_cast<Function>(GA->getAliasee());

    return nullptr;
  }

  // Step 2. check callee MD
  auto *Node = CB.getMetadata(LLVMContext::MD_callees);
  if (Node)
    return mdconst::extract<Function>(Node->getOperand(0));

  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return nullptr;

  if (isa<LoadInst>(I)) {
    std::set<Function *> TargetSet;

    Node = findTBAA(*I);
    if (Node) {
      // Step 3. check VTMap
      if (Node->isTBAAVtableAccess()) {
        TargetSet = VTMap[I];
        if (TargetSet.size() > 0)
          return *(TargetSet.begin());
        return nullptr;
      }
      // Step 4. check InstMap with TBAA and Function Type
      TargetSet = InstMap[Node][CV->getType()];
      if (TargetSet.size() > 0)
        return *(TargetSet.begin());
    }

    // Step 5. check Globalmap with Function type
    auto *Op0 = I->getOperand(0);
    if (!Op0)
      return nullptr;

    Op0 = Op0->stripPointerCasts();
    if (!Op0)
      return nullptr;

    auto *Op0I = dyn_cast<Instruction>(Op0);
    if (!Op0I) {
      if (auto *CE = dyn_cast<ConstantExpr>(Op0))
        Op0I = CE->getAsInstruction();
      if (!Op0I)
        return nullptr;
    }

    if (auto *GEPI = dyn_cast<GetElementPtrInst>(Op0I)) {
      ConstantInt *CI;
      if (GEPI->getNumIndices() < 2) {
        auto *GEPIOp1 = GEPI->getOperand(1);
        if (!GEPIOp1)
          return nullptr;

        CI = dyn_cast<ConstantInt>(GEPIOp1);
      } else {
        auto *GEPIOp2 = GEPI->getOperand(2);
        if (!GEPIOp2)
          return nullptr;

        CI = dyn_cast<ConstantInt>(GEPIOp2);
      }
      if (!CI)
        return nullptr;

      auto *GEPIOp0 = GEPI->getOperand(0);
      if (!GEPIOp0)
        return nullptr;

      TargetSet = GlobalMap[GEPIOp0->getType()][CI->getSExtValue()];
      if (TargetSet.size() > 0)
        return *(TargetSet.begin());
    } else {
      TargetSet = GlobalMap[Op0I->getType()][0];
      if (TargetSet.size() > 0)
        return *(TargetSet.begin());
    }
  }

  return nullptr;
}

MDNode *IndCallSolverImpl::findTBAA(Instruction &I) const {
  auto *Node = I.getMetadata(LLVMContext::MD_tbaa);
  if (Node)
    return Node;

  auto *Op = I.getOperand(0)->stripPointerCasts();
  if (!Op || !isa<Instruction>(Op))
    return nullptr;

  auto *NI = dyn_cast<Instruction>(Op);
  if (!isa<LoadInst>(NI) && !isa<GetElementPtrInst>(NI))
    return nullptr;

  return findTBAA(*NI);
}

void IndCallSolverImpl::findDeVirtualizableCalls(
    SmallVectorImpl<DevirtCallSite> &DevirtCalls, Value &V, uint64_t Offset,
    Module &M) const {
  for (const Use &U : V.uses()) {
    Value *User = U.getUser();
    if (!User)
      continue;

    if (isa<BitCastInst>(User))
      findDeVirtualizableCalls(DevirtCalls, *User, Offset, M);
    else if (isa<LoadInst>(User))
      findCalledValue(DevirtCalls, *User, Offset);
    else if (auto *GEPI = dyn_cast<GetElementPtrInst>(User)) {
      SmallVector<Value *, 8> Indices(GEPI->op_begin() + 1, GEPI->op_end());
      int64_t GEPOffset = M.getDataLayout().getIndexedOffsetInType(
          GEPI->getSourceElementType(), Indices);
      findDeVirtualizableCalls(DevirtCalls, *User, Offset + GEPOffset, M);
    }
  }
}

void IndCallSolverImpl::findCalledValue(
    SmallVectorImpl<DevirtCallSite> &DevirtCalls, Value &V,
    uint64_t Offset) const {
  for (const Use &U : V.uses()) {
    Value *User = U.getUser();
    if (!User)
      continue;

    if (isa<BitCastInst>(User))
      return findCalledValue(DevirtCalls, *User, Offset);
    if (CallSite CS = CallSite(User)) {
      if (CS.getCalledFunction())
        continue;
      DevirtCalls.push_back({CS.getCalledValue(), Offset});
      return;
    }
  }
}

} // end namespace ftg
