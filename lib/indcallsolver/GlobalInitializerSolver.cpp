#include "ftg/indcallsolver/GlobalInitializerSolver.h"
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

using namespace ftg;
using namespace llvm;

GlobalInitializerSolver::GlobalInitializerSolver(
    GlobalInitializerSolverHandler &&Handler)
    : Handler(std::move(Handler)) {}

std::set<const llvm::Function *>
GlobalInitializerSolver::solve(const llvm::CallBase &CB) const {
  const auto *CO = CB.getCalledOperand();
  if (!CO)
    return {};

  const auto *LI = dyn_cast_or_null<LoadInst>(CO->stripPointerCasts());
  if (!LI)
    return {};

  const auto *Ptr = LI->getPointerOperand();
  assert(Ptr && "Unexpected LLVM API Behavior");

  Ptr = Ptr->stripPointerCasts();
  assert(Ptr && "Unexpected LLVM API Behavior");

  if (const auto *CE = dyn_cast<ConstantExpr>(Ptr)) {
    Ptr = CE->getAsInstruction();
  }

  if (!isa<Instruction>(Ptr))
    return {};

  const auto *GEPI = dyn_cast_or_null<GetElementPtrInst>(Ptr);
  if (!GEPI) {
    const auto *Ty = Ptr->getType();
    return Handler.get(Ty, 0);
  }

  Ptr = GEPI->getOperand(0);
  if (!Ptr)
    return {};

  const ConstantInt *CI = nullptr;
  unsigned NumIndices = GEPI->getNumIndices();
  if (NumIndices == 1)
    CI = dyn_cast_or_null<ConstantInt>(GEPI->getOperand(1));
  else if (NumIndices > 1)
    CI = dyn_cast_or_null<ConstantInt>(GEPI->getOperand(2));
  if (!CI)
    return {};

  return Handler.get(Ptr->getType(), CI->getSExtValue());
}
