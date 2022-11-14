#ifndef FTG_INDCALLSOLVER_INDCALLSOLVERMGR_H
#define FTG_INDCALLSOLVER_INDCALLSOLVERMGR_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include <set>

namespace ftg {

class IndCallSolverMgr {

public:
  void solve(llvm::Module &M);
  const llvm::Function *getCalledFunction(const llvm::CallBase &CB) const;
  virtual std::set<const llvm::Function *>
  getCalledFunctions(const llvm::CallBase &CB) const;

private:
  std::vector<std::unique_ptr<IndCallSolver>> Solvers;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_INDCALLSOLVERMGR_H
