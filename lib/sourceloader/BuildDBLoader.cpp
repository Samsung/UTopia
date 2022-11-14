//===-- BuildDBLoader.cpp - Implementation of BuildDBLoader ---------------===//

#include "ftg/sourceloader/BuildDBLoader.h"
#include "ftg/sourceloader/BuildDB.h"

#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "llvm/IRReader/IRReader.h"

using namespace ftg;

BuildDBLoader::BuildDBLoader(std::string BuildDBPath)
    : BuildDBPath(std::move(BuildDBPath)) {}

std::unique_ptr<SourceCollection> BuildDBLoader::load() {
  std::unique_ptr<const BuildDB> DB = BuildDB::fromJson(BuildDBPath);
  assert(DB && "Failed to load Build DB");

  // load AST
  std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits;
  for (auto ASTPath : DB->getASTPaths()) {
    auto ASTUnit = clang::ASTUnit::LoadFromASTFile(
        ASTPath, clang::RawPCHContainerReader(), clang::ASTUnit::LoadASTOnly,
        clang::CompilerInstance::createDiagnostics(
            new clang::DiagnosticOptions()),
        clang::FileSystemOptions());
    assert(ASTUnit && "AST load failed");
    ASTUnits.push_back(std::move(ASTUnit));
  }

  // load IR
  auto LLVMContext = std::make_unique<llvm::LLVMContext>();
  llvm::SMDiagnostic SmDiagnostic;
  auto LLVMModule =
      llvm::parseIRFile(DB->getBCPath(), SmDiagnostic, *LLVMContext);
  assert((LLVMContext && LLVMModule) && "BC load failed");

  return std::make_unique<SourceCollection>(
      std::move(LLVMModule), std::move(LLVMContext), std::move(ASTUnits),
      DB->getProjectDir());
}
