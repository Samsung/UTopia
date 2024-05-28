#include "ftg/indcallsolver/TBAAVirtSolver.h"
#include "ftg/utils/StringUtil.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>

using namespace ftg;
using namespace llvm;

TBAAVirtSolver::TBAAVirtSolver(TBAAVirtSolverHandler &&Handler)
    : Handler(std::move(Handler)) {}

std::set<const Function *> TBAAVirtSolver::solve(const CallBase &CB) const {
  const MDNode *TBAA = nullptr;
  unsigned Index = 0;
  const auto *CO = CB.getCalledOperand();
  assert(CO && "Unexpected LLVM API Behavior");

  const auto *T = CO->getType();
  if (!T || T->getNumContainedTypes() < 1)
    return {};

  const auto *CT = dyn_cast_or_null<FunctionType>(T->getContainedType(0));
  if (!CT)
    return {};

  if (!isVirtCall(CB, &TBAA, &Index))
    return {};

  auto VirtTables = Handler.get(TBAA);
  if (VirtTables.size() == 0)
    return {};

  std::set<const Function *> Result;
  for (const auto *VirtTable : VirtTables) {
    const auto *T = VirtTable->getType();
    assert(T && "Unexpected LLVM API Behavior");
    assert(T->isArrayTy() && "Unexpected Program State");

    if (Index + 2 >= T->getArrayNumElements())
      continue;

    const auto *E = VirtTable->getAggregateElement(Index + 2);
    assert(E && "Unexpected LLVM API Behavior");

    const auto *F = dyn_cast<Function>(E->stripPointerCasts());
    assert(F && "Unexpected LLVM API Behavior");

    if (F->getName() == "__cxa_pure_virtual")
      continue;

    const auto *FT = F->getFunctionType();
    assert(FT && "Unexpected LLVM API Behavior");

    if (!isCallableType(*CT, *FT))
      continue;

    Result.emplace(F);
  }
  return Result;
}

bool TBAAVirtSolver::isCallableType(const FunctionType &CallerType,
                                    const FunctionType &CalleeType) const {
  auto NumParams = CallerType.getNumParams();
  if (NumParams != CalleeType.getNumParams())
    return false;

  // NOTE: This logic considers given types are for class methods, and skips
  // comparing class instance type because there is no way to identify class
  // hierarhcy. For example, if class2 inherits class1 than class1 type can be
  // used to call calss2 type, thus this should be considered to determine
  // callee-caller callable.
  for (unsigned S = 1; S < NumParams; ++S) {
    if (CallerType.getParamType(S) != CalleeType.getParamType(S))
      return false;
  }
  return true;
}

bool TBAAVirtSolver::isVirtCall(const llvm::CallBase &CB,
                                const llvm::MDNode **TBAA,
                                unsigned *Index) const {
  const auto *LI = dyn_cast_or_null<LoadInst>(CB.getCalledOperand());
  if (!LI)
    return false;

  const auto *GEPI =
      dyn_cast_or_null<GetElementPtrInst>(LI->getPointerOperand());
  if (!GEPI || GEPI->getNumIndices() != 1)
    return false;

  const auto *CI = dyn_cast_or_null<ConstantInt>(GEPI->idx_begin());
  if (!CI)
    return false;

  LI = dyn_cast_or_null<LoadInst>(GEPI->getPointerOperand());
  if (!LI)
    return false;

  const auto *MD = LI->getMetadata(LLVMContext::MD_tbaa);
  if (!MD || !MD->isTBAAVtableAccess())
    return false;

  if (TBAA)
    *TBAA = MD;

  if (Index)
    *Index = CI->getZExtValue();

  return true;
}
