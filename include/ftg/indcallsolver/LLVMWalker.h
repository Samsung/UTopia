#ifndef FTG_INDCALLSOLVER_LLVMWALKER_H
#define FTG_INDCALLSOLVER_LLVMWALKER_H

#include "ftg/indcallsolver/LLVMWalkHandler.h"
#include <llvm/IR/Module.h>

namespace ftg {

class LLVMWalker {
public:
  void addHandler(LLVMWalkHandler<llvm::GlobalVariable> *Handler);
  void addHandler(LLVMWalkHandler<llvm::Instruction> *Handler);
  void walk(const llvm::Module &M);

private:
  std::vector<LLVMWalkHandler<llvm::GlobalVariable> *> GVHandlers;
  std::vector<LLVMWalkHandler<llvm::Instruction> *> InstHandlers;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_LLVMWALKER_H
