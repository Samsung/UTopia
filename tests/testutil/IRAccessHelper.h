#ifndef FTG_TESTS_IRACCESSHELPER_H
#define FTG_TESTS_IRACCESSHELPER_H

#include "ftg/sourceloader/SourceCollection.h"

namespace ftg {

class IRAccessHelper {

public:
  IRAccessHelper(const llvm::Module &M);
  llvm::Module &getModule();
  llvm::Function *getFunction(std::string Name);
  llvm::BasicBlock *getBasicBlock(std::string FuncName, unsigned BIdx);
  llvm::Instruction *getInstruction(std::string FuncName, unsigned BIdx,
                                    unsigned IIdx);
  llvm::GlobalVariable *getGlobalVariable(std::string Name);

private:
  const llvm::Module &M;

  std::map<const llvm::Function *, std::vector<const llvm::BasicBlock *>>
      BasicBlocks;
  std::map<const llvm::BasicBlock *, std::vector<const llvm::Instruction *>>
      Instructions;
};

} // namespace ftg

#endif // FTG_TESTS_IRACCESSHELPER_H
