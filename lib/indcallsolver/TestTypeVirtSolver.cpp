#include "ftg/indcallsolver/TestTypeVirtSolver.h"
#include "ftg/indcallsolver/IndCallSolverUtil.hpp"
#include <llvm/Analysis/TypeMetadataUtils.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Metadata.h>

using namespace ftg;
using namespace llvm;

TestTypeVirtSolver::TestTypeVirtSolver(TestTypeVirtSolverHandler &&Handler)
    : Handler(std::move(Handler)) {}

std::set<const llvm::Function *>
TestTypeVirtSolver::solve(const CallBase &CB) const {
  const auto *CO = CB.getCalledOperand();
  if (!CO)
    return {};

  const auto *I = dyn_cast_or_null<Instruction>(CO->stripPointerCasts());
  const auto *TBAA = getTBAA(I);
  if (!TBAA || !TBAA->isTBAAVtableAccess())
    return {};

  return Handler.get(I);
}
