#ifndef FTG_INDCALLSOLVER_TBAASIMPLESOLVER_H
#define FTG_INDCALLSOLVER_TBAASIMPLESOLVER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/indcallsolver/TBAASimpleSolverHandler.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <set>

namespace ftg {

class TBAASimpleSolver : public IndCallSolver {
public:
  TBAASimpleSolver() = default;
  TBAASimpleSolver(TBAASimpleSolverHandler &&Handler);
  std::set<const llvm::Function *>
  solve(const llvm::CallBase &CB) const override;

private:
  TBAASimpleSolverHandler Handler;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TBAASIMPLESOLVER_H
