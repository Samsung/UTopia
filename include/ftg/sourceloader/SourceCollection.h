//===-- SourceCollection.h - data class for sources -------------*- C++ -*-===//
///
/// \file
/// Defines data class to store target sources(AST/BC) of analysis.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_SOURCELOADER_SOURCECOLLECTION_H
#define FTG_SOURCELOADER_SOURCECOLLECTION_H

#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <string>
#include <vector>

namespace ftg {

/// Data class to store target sources(AST/BC) of analysis.
/// Advised to create with ftg::sourceloader interface.
class SourceCollection {
private:
  std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits;
  std::unique_ptr<llvm::Module> LLVMModule;
  std::unique_ptr<llvm::LLVMContext> LLVMContext;
  std::string BaseDir;

public:
  SourceCollection(std::unique_ptr<llvm::Module> LLVMModule,
                   std::unique_ptr<llvm::LLVMContext> LLVMContext,
                   std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits,
                   std::string BaseDir)
      : ASTUnits(std::move(ASTUnits)), LLVMModule(std::move(LLVMModule)),
        LLVMContext(std::move(LLVMContext)), BaseDir(BaseDir) {}
  SourceCollection(const SourceCollection &) = delete;
  SourceCollection &operator=(const SourceCollection &) = delete;
  ~SourceCollection();
  const std::vector<clang::ASTUnit *> getASTUnits() const;
  const llvm::Module &getLLVMModule() const;
  const llvm::LLVMContext &getLLVMContext() const;
  const std::string getBaseDir() const;
};

} // namespace ftg

#endif // FTG_SOURCELOADER_SOURCECOLLECTION_H
