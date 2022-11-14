#ifndef FTG_INDCALLSOLVER_LLVMWALKHANDLER_H
#define FTG_INDCALLSOLVER_LLVMWALKHANDLER_H

#include <llvm/IR/GlobalVariable.h>

namespace ftg {

template <typename T> class LLVMWalkHandler {
public:
  virtual void handle(const T &IR) = 0;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_LLVMWALKHANDLER_H
