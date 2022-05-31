//===-- SourceCollection.cpp - Implementation of SourceCollection ---------===//

#include "ftg/sourceloader/SourceCollection.h"

using namespace ftg;

SourceCollection::~SourceCollection() {
  ASTUnits.clear();
  // LLVMModule should be deleted before LLVMContext.
  LLVMModule.reset();
  LLVMContext.reset();
}

const std::vector<clang::ASTUnit *> SourceCollection::getASTUnits() const {
  std::vector<clang::ASTUnit *> RefASTUnits;
  for (auto &Unit : ASTUnits) {
    RefASTUnits.push_back(Unit.get());
  }
  return RefASTUnits;
}
const llvm::Module &SourceCollection::getLLVMModule() const {
  return *LLVMModule;
}
const llvm::LLVMContext &SourceCollection::getLLVMContext() const {
  return *LLVMContext;
}
const std::string SourceCollection::getBaseDir() const { return BaseDir; }
