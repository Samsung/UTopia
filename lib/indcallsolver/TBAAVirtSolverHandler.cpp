#include "ftg/indcallsolver/TBAAVirtSolverHandler.h"
#include "ftg/utils/StringUtil.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/Instructions.h>

using namespace ftg;
using namespace llvm;

static inline bool isVirtTable(const GlobalVariable &GV) {
  auto Name = GV.getName();
  auto DemangledName = util::getDemangledName(Name.str());
  return DemangledName.find("vtable for ") == 0;
}

static bool isGEPWithNoNotionalOverIndexing(const ConstantExpr* CE) {
  if (CE->getOpcode() != Instruction::GetElementPtr) return false;

  gep_type_iterator GEPI = gep_type_begin(CE), E = gep_type_end(CE);
  User::const_op_iterator OI = std::next(CE->op_begin());

  // Skip the first index, as it has no static limit.
  ++GEPI;
  ++OI;

  // The remaining indices must be compile-time known integers within the
  // bounds of the corresponding notional static array types.
  for (; GEPI != E; ++GEPI, ++OI) {
    ConstantInt *CI = dyn_cast<ConstantInt>(*OI);
    if (!CI) return false;
    if (ArrayType *ATy = dyn_cast<ArrayType>(GEPI.getIndexedType()))
      if (CI->getValue().getActiveBits() > 64 ||
          CI->getZExtValue() >= ATy->getNumElements())
        return false;
  }
  // All the indices checked out.
  return true;
}

TBAAVirtSolverHandler::TBAAVirtSolverHandler(TBAAVirtSolverHandler &&Handler)
    : Map(std::move(Handler.Map)) {}

std::set<const llvm::Constant *>
TBAAVirtSolverHandler::get(const llvm::MDNode *Node) const {
  auto Iter = Map.find(Node);
  if (Iter == Map.end())
    return {};

  return Iter->second;
}

void TBAAVirtSolverHandler::handle(const llvm::Instruction &I) {
  const auto *SI = dyn_cast<StoreInst>(&I);
  if (!SI)
    return;

  const auto *TBAA = I.getMetadata(LLVMContext::MD_tbaa);
  if (!TBAA)
    return;

  const auto *ValuePtr = SI->getValueOperand();
  assert(ValuePtr && "Unexpected LLVM API Behavior");

  const auto *CE =
      dyn_cast_or_null<ConstantExpr>(ValuePtr->stripPointerCasts());
  if (!CE || !isGEPWithNoNotionalOverIndexing(CE))
    return;

  const auto *VirtTableGV = dyn_cast_or_null<GlobalVariable>(CE->getOperand(0));
  if (!VirtTableGV || !isVirtTable(*VirtTableGV))
    return;

  if (!VirtTableGV->hasInitializer())
    return;

  const auto *Init =
      dyn_cast_or_null<ConstantStruct>(VirtTableGV->getInitializer());
  if (!Init)
    return;

  const auto *StTy = dyn_cast_or_null<StructType>(Init->getType());
  if (!StTy || StTy->getNumElements() != 1)
    return;

  const auto *VirtTable = Init->getAggregateElement((unsigned)0);
  if (!VirtTable)
    return;

  Map[TBAA].emplace(VirtTable);
}
