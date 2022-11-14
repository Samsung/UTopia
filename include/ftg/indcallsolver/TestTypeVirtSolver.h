#ifndef FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVER_H
#define FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/indcallsolver/TestTypeVirtSolverHandler.h"
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Module.h>
#include <set>

namespace ftg {

class TestTypeVirtSolver : public IndCallSolver {
public:
  TestTypeVirtSolver(TestTypeVirtSolverHandler &&Handler);
  std::set<const llvm::Function *>
  solve(const llvm::CallBase &CB) const override;

private:
  TestTypeVirtSolverHandler Handler;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TESTTYPEVIRTSOLVER_H
