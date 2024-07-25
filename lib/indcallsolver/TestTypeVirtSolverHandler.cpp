#include "ftg/indcallsolver/TestTypeVirtSolverHandler.h"
#include <llvm/Analysis/TypeMetadataUtils.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>

using namespace ftg;
using namespace llvm;

namespace {

struct DevirtCallBase {
  // Called Operand
  const Value *CO;
  // Offset at VTable
  uint64_t VTOffset;

  DevirtCallBase(const Value *V, uint64_t Offset) : CO(V), VTOffset(Offset) {}
};

void collectDevirtCallBases(std::vector<DevirtCallBase> &Bases, const Value &V,
                            uint64_t Offset) {
  for (const Use &U : V.uses()) {
    const auto *User = U.getUser();
    if (!User)
      continue;

    if (isa<BitCastInst>(User))
      return collectDevirtCallBases(Bases, *User, Offset);

    if (const auto *CB = dyn_cast<CallBase>(User)) {
      if (CB->getCalledFunction())
        continue;

      Bases.emplace_back(CB->getCalledOperand(), Offset);
      return;
    }
  }
} // namespace

void collectDevirtCallBases(std::vector<DevirtCallBase> &Bases, const Value &V,
                            uint64_t Offset, const Module &M) {
  for (const auto &U : V.uses()) {
    const auto *User = U.getUser();
    if (!User)
      continue;

    if (isa<BitCastInst>(User)) {
      collectDevirtCallBases(Bases, *User, Offset, M);
    } else if (isa<LoadInst>(User)) {
      collectDevirtCallBases(Bases, *User, Offset);
    } else if (const auto *GEPI = dyn_cast_or_null<GetElementPtrInst>(User)) {
      SmallVector<Value *, 8> Indices(GEPI->op_begin() + 1, GEPI->op_end());
      auto GEPOffset = M.getDataLayout().getIndexedOffsetInType(
          GEPI->getSourceElementType(), Indices);
      collectDevirtCallBases(Bases, *User, Offset + GEPOffset, M);
    }
  }
}
} // namespace

TestTypeVirtSolverHandler::TestTypeVirtSolverHandler(
    TestTypeVirtSolverHandler &&Handler) {
  this->Map = std::move(Handler.Map);
}

void TestTypeVirtSolverHandler::collect(const Module &M) {
  const auto *TypeTestFunc = Intrinsic::getDeclaration(
      const_cast<llvm::Module *>(&M), Intrinsic::type_test);
  if (!TypeTestFunc)
    return;

  for (const auto &U : TypeTestFunc->uses()) {
    const auto *CB = dyn_cast_or_null<CallBase>(U.getUser());
    if (!CB)
      continue;

    const auto *Op0 = CB->getArgOperand(0);
    assert(Op0 && "Unexpected LLVM API Behavior");

    Op0 = Op0->stripPointerCasts();
    assert(Op0 && "Unexpected LLVM API Behavior");

    std::vector<DevirtCallBase> DCBases;
    collectDevirtCallBases(DCBases, *Op0, 0, M);

    for (const auto &DCBase : DCBases) {
      const auto *MDValue =
          dyn_cast_or_null<MetadataAsValue>(CB->getArgOperand(1));
      if (!MDValue)
        continue;

      for (const auto &TM : TypeIDMap[MDValue->getMetadata()]) {
        const auto *Ptr = getPointerAtOffset(
            const_cast<Constant *>(TM.GV->getInitializer()),
            TM.GVOffset + DCBase.VTOffset, *const_cast<Module *>(&M));
        if (!Ptr)
          continue;

        const auto *F = dyn_cast_or_null<Function>(Ptr->stripPointerCasts());
        if (!F)
          continue;

        Map[DCBase.CO].emplace(F);
      }
    }
  }
}

std::set<const Function *>
TestTypeVirtSolverHandler::get(const Value *V) const {
  auto Iter = Map.find(V);
  if (Iter == Map.end())
    return {};

  return Iter->second;
}

void TestTypeVirtSolverHandler::handle(const GlobalVariable &GV) {
  SmallVector<MDNode *, 2> MDTypes;
  GV.getMetadata(LLVMContext::MD_type, MDTypes);
  if (MDTypes.empty() || GV.isDeclaration() || !GV.isConstant() ||
      !GV.hasInitializer())
    return;

  for (const auto *MDType : MDTypes) {
    if (!MDType || MDType->getNumOperands() < 2)
      continue;

    const auto *MDTypeID = MDType->getOperand(1).get();
    const auto *ConstantMD =
        dyn_cast_or_null<ConstantAsMetadata>(MDType->getOperand(0));
    if (!ConstantMD)
      continue;

    const auto *CI = dyn_cast_or_null<ConstantInt>(ConstantMD->getValue());
    if (!CI)
      continue;

    TypeIDMap[MDTypeID].emplace(TypeMetaInfo(&GV, CI->getZExtValue()));
  }
}
