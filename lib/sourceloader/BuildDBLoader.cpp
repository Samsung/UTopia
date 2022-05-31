//===-- BuildDBLoader.cpp - Implementation of BuildDBLoader ---------------===//

#include "ftg/sourceloader/BuildDBLoader.h"

#include "ftg/sourceloader/BuildDB.h"

#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "llvm/IRReader/IRReader.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using namespace ftg;

BuildDBLoader::BuildDBLoader(std::string BuildDBPath, std::string BinaryName)
    : BinaryName(BinaryName) {
  loadBuildDB(BuildDBPath);
}

std::unique_ptr<SourceCollection> BuildDBLoader::load() {
  // load AST
  std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits;
  for (auto ASTPath : getASTPaths()) {
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
  auto LLVMModule = llvm::parseIRFile(getBCPath(), SmDiagnostic, *LLVMContext);
  assert((LLVMContext || LLVMModule) && "BC load failed");

  return std::make_unique<SourceCollection>(
      std::move(LLVMModule), std::move(LLVMContext), std::move(ASTUnits),
      PE->ProjectDir);
}

std::string BuildDBLoader::getBCPath() {
  auto BCPath = PE->getBCInfo(BinaryName)->BCFile;
  assert(!BCPath.empty() && "project_entry.json is not valid");
  return BCPath;
}

std::vector<std::string> BuildDBLoader::getASTPaths() {
  std::vector<std::string> ASTPaths;
  for (auto &ObjInfo : DB->Compiles) {
    auto ASTPath = ObjInfo.ASTFile;
    if (!ASTPath.empty())
      ASTPaths.push_back(ASTPath);
  }
  assert(!ASTPaths.empty() && "compiles.json is not valid");
  return ASTPaths;
}

void BuildDBLoader::loadBuildDB(std::string BuildDBPath) {
  PE = ProjectEntry::fromJson(BuildDBPath);
  assert(PE && "project_entry.json not exists");

  auto CompileDBPath = fs::path(BuildDBPath).parent_path() /
                       ("compiles_" + BinaryName + ".json");
  DB = CompileDB::fromJson(CompileDBPath);
  assert(DB && "compiles.json not exists");
}
