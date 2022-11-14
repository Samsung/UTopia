#ifndef FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVER_H
#define FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVER_H

#include "ftg/indcallsolver/GlobalInitializerSolverHandler.h"
#include "ftg/indcallsolver/IndCallSolver.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <set>

namespace ftg {

class GlobalInitializerSolver : public IndCallSolver {
public:
  GlobalInitializerSolver(GlobalInitializerSolverHandler &&Handler);
  std::set<const llvm::Function *>
  solve(const llvm::CallBase &CB) const override;

private:
  GlobalInitializerSolverHandler Handler;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVER_H
