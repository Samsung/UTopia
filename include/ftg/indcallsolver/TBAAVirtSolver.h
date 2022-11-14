#ifndef FTG_INDCALLSOLVER_TBAAVIRTSOLVER_H
#define FTG_INDCALLSOLVER_TBAAVIRTSOLVER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/indcallsolver/TBAAVirtSolverHandler.h"
#include <llvm/IR/Module.h>

namespace ftg {

class TBAAVirtSolver : public IndCallSolver {
public:
  TBAAVirtSolver(TBAAVirtSolverHandler &&Handler);
  std::set<const llvm::Function *>
  solve(const llvm::CallBase &CB) const override;

private:
  TBAAVirtSolverHandler Handler;
  bool isCallableType(const llvm::FunctionType &CallerType,
                      const llvm::FunctionType &CalleeType) const;
  bool isVirtCall(const llvm::CallBase &CB, const llvm::MDNode **TBAA = nullptr,
                  unsigned *Index = nullptr) const;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TBAAVIRTSOLVER_H
