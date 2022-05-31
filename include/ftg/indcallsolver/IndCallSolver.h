#ifndef FTG_INDCALLSOLVER_INDCALLSOLVER_H
#define FTG_INDCALLSOLVER_INDCALLSOLVER_H

#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"

namespace ftg {

class IndCallSolver {

public:
  virtual ~IndCallSolver() = default;
  virtual void solve(llvm::Module &M) = 0;
  virtual llvm::Function *getCalledFunction(llvm::CallBase &CB) = 0;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_INDCALLSOLVER_H
