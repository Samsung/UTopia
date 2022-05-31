#include "IRAccessHelper.h"

namespace ftg {

IRAccessHelper::IRAccessHelper(const llvm::Module &M) : M(M) {
  for (const auto &F : M) {
    std::vector<const llvm::BasicBlock *> Bs;
    for (const auto &BB : F) {
      Bs.push_back(&BB);
      std::vector<const llvm::Instruction *> Is;
      for (const auto &I : BB)
        Is.push_back(&I);
      Instructions.emplace(&BB, Is);
    }
    BasicBlocks.emplace(&F, Bs);
  }
}

llvm::Module &IRAccessHelper::getModule() {
  return *const_cast<llvm::Module *>(&M);
}

llvm::Function *IRAccessHelper::getFunction(std::string Name) {
  return getModule().getFunction(Name);
}

llvm::BasicBlock *IRAccessHelper::getBasicBlock(std::string Name,
                                                unsigned BIdx) {
  auto *F = getFunction(Name);
  if (!F)
    return nullptr;

  auto Iter = BasicBlocks.find(F);
  if (Iter == BasicBlocks.end())
    return nullptr;

  auto &BasicBlocks = Iter->second;
  if (BasicBlocks.size() < BIdx)
    return nullptr;

  auto *BB = BasicBlocks[BIdx];
  if (!BB)
    return nullptr;

  return const_cast<llvm::BasicBlock *>(BB);
}

llvm::Instruction *
IRAccessHelper::getInstruction(std::string Name, unsigned BIdx, unsigned IIdx) {
  auto *B = getBasicBlock(Name, BIdx);
  if (!B)
    return nullptr;

  auto Iter = Instructions.find(B);
  if (Iter == Instructions.end())
    return nullptr;

  auto &Instructions = Iter->second;
  if (Instructions.size() < IIdx)
    return nullptr;

  const auto *I = Instructions[IIdx];
  if (!I)
    return nullptr;

  return const_cast<llvm::Instruction *>(I);
}

llvm::GlobalVariable *IRAccessHelper::getGlobalVariable(std::string Name) {
  auto *GV = M.getGlobalVariable(Name, true);
  if (!GV)
    return nullptr;

  return const_cast<llvm::GlobalVariable *>(GV);
}

} // namespace ftg
