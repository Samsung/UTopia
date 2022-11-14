#include "ftg/indcallsolver/LLVMWalker.h"

using namespace ftg;
using namespace llvm;

void LLVMWalker::addHandler(LLVMWalkHandler<GlobalVariable> *Handler) {
  GVHandlers.emplace_back(Handler);
}

void LLVMWalker::addHandler(LLVMWalkHandler<Instruction> *Handler) {
  InstHandlers.emplace_back(Handler);
}

void LLVMWalker::walk(const llvm::Module &M) {
  if (GVHandlers.size() > 0) {
    for (const auto &GV : M.globals()) {
      for (auto *Handler : GVHandlers) {
        assert(Handler && "Unexpected Program State");
        Handler->handle(GV);
      }
    }
  }

  if (InstHandlers.size() > 0) {
    for (const auto &F : M) {
      for (const auto &B : F) {
        for (const auto &I : B) {
          for (auto *Handler : InstHandlers) {
            assert(Handler && "Unexpected Program State");
            Handler->handle(I);
          }
        }
      }
    }
  }
}
