#include "ftg/indcallsolver/TBAASimpleSolver.h"
#include "ftg/indcallsolver/IndCallSolverUtil.hpp"
#include <llvm/IR/Instructions.h>

using namespace ftg;
using namespace llvm;

TBAASimpleSolver::TBAASimpleSolver(TBAASimpleSolverHandler &&Handler)
    : Handler(std::move(Handler)) {}

std::set<const Function *> TBAASimpleSolver::solve(const CallBase &CB) const {
  const auto *CO = CB.getCalledOperand();
  if (!CO)
    return {};

  const auto *TBAA =
      getTBAA(dyn_cast_or_null<Instruction>(CO->stripPointerCasts()));
  if (!TBAA)
    return {};

  return Handler.get(TBAA, CO->getType());
}
