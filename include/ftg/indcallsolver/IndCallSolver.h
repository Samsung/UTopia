#ifndef FTG_INDCALLSOLVER_INDCALLSOLVER_H
#define FTG_INDCALLSOLVER_INDCALLSOLVER_H

#include "llvm/IR/InstrTypes.h"
#include <set>

namespace ftg {

class IndCallSolver {

public:
  virtual ~IndCallSolver() = default;
  virtual std::set<const llvm::Function *>
  solve(const llvm::CallBase &CB) const = 0;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_INDCALLSOLVER_H
