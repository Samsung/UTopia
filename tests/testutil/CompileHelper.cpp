#include "CompileHelper.h"
#include "TestUtil.h"
#include "ftg/utils/FileUtil.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Serialization/PCHContainerOperations.h"
#include "llvm/IRReader/IRReader.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

CompileHelper::CompileHelper(std::string SrcDir) {
  if (SrcDir.empty())
    SrcDir = getTmpDirPath();
  this->SrcDir = SrcDir;
};

std::string CompileHelper::getCompiler(SourceType Type) {

  std::string Compiler;

  switch (Type) {
  case SourceType_C:
    Compiler = "clang";
    break;
  case SourceType_CPP:
    Compiler = "clang++";
    break;
  case SourceType_HEADER:
    Compiler = "";
    break;
  default:
    break;
  }

  return Compiler;
}

std::string CompileHelper::getFileExt(SourceType Type) {

  std::string Result = "";

  switch (Type) {
  case SourceType_C:
    Result = "c";
    break;
  case SourceType_CPP:
    Result = "cpp";
    break;
  case SourceType_HEADER:
    Result = "h";
    break;
  default:
    break;
  }

  return Result;
}

std::unique_ptr<SourceCollection> CompileHelper::load() {
  if (!IRFile)
    return nullptr;

  std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits;
  for (auto &ASTFile : ASTFiles) {
    if (!ASTFile.Ptr)
      continue;
    ASTUnits.emplace_back(std::move(ASTFile.Ptr));
  }
  return std::make_unique<SourceCollection>(std::move(IRFile->Ptr),
                                            std::move(IRFile->Ctx),
                                            std::move(ASTUnits), SrcDir);
}

CompileHelper::~CompileHelper() {
  for (auto &Iter : SourceFiles) {
    if (Iter.Generated && fs::exists(Iter.Path))
      fs::remove(Iter.Path);
  }
  SourceFiles.clear();
  ASTFiles.clear();
  IRFile.reset();
}

bool CompileHelper::add(const std::string Code, const std::string Name,
                        const std::string Opt, SourceType Type) {

  auto Path = getUniqueFilePath(SrcDir, Name, getFileExt(Type));
  if (!util::saveFile(Path.c_str(), Code.c_str()))
    return false;
  SourceFiles.emplace_back(Name, Path, Opt, Type);
  return true;
}

bool CompileHelper::add(const std::string Path, const std::string Opt,
                        SourceType Type) {

  SourceFiles.emplace_back(Path, Opt, Type);
  return true;
}

bool CompileHelper::compileAST() {

  for (auto &SourceFile : SourceFiles) {
    ASTFiles.emplace_back(SourceFile);
    if (!ASTFiles.back().Ptr) {
      ASTFiles.clear();
      return false;
    }
  }
  return true;
}

bool CompileHelper::compileIR() {

  IRFile = std::make_unique<IRData>(SourceFiles);
  if (!IRFile || !IRFile->Ptr)
    return false;
  return true;
}

std::vector<clang::ASTUnit *> CompileHelper::getASTUnits() {

  std::vector<clang::ASTUnit *> ASTUnits;
  for (auto &ASTFile : ASTFiles) {
    if (!ASTFile.Ptr) {
      ASTUnits.clear();
      return ASTUnits;
    }
    ASTUnits.emplace_back(ASTFile.Ptr.get());
  }
  return ASTUnits;
}

std::vector<std::unique_ptr<clang::ASTUnit>> CompileHelper::takeASTUnits() {

  std::vector<std::unique_ptr<clang::ASTUnit>> ASTUnits;
  for (auto &ASTFile : ASTFiles) {
    if (!ASTFile.Ptr)
      continue;
    ASTUnits.emplace_back(std::move(ASTFile.Ptr));
  }
  return ASTUnits;
}

llvm::Module *CompileHelper::getLLVMModule() {

  if (!IRFile)
    return nullptr;
  return IRFile->Ptr.get();
}

std::string CompileHelper::getIRPath() const {

  if (!IRFile)
    return "";
  return IRFile->Path;
}

void CompileHelper::printASTUnits() const {

  for (auto &Data : ASTFiles) {
    if (!Data.Ptr)
      continue;
    Data.Ptr->getASTContext().getTranslationUnitDecl()->dump(llvm::outs(),
                                                             true);
  }
}

void CompileHelper::printLLVMModule() const {

  if (!IRFile || !IRFile->Ptr)
    return;
  llvm::outs() << *IRFile->Ptr << "\n";
}

CompileHelper::SourceFileData::SourceFileData(const std::string Name,
                                              const std::string Path,
                                              const std::string Opt,
                                              SourceType Type)
    : Name(Name), Path(Path), Opt(Opt), Type(Type), Generated(true) {}

CompileHelper::SourceFileData::SourceFileData(const std::string Path,
                                              const std::string Opt,
                                              SourceType Type)
    : Name("temp"), Path(Path), Opt(Opt), Type(Type), Generated(false) {}

CompileHelper::ASTData::ASTData(const SourceFileData &SourceFile)
    : Path(SourceFile.Path + ".ast") {

  auto Compiler = CompileHelper::getCompiler(SourceFile.Type);
  if (Compiler.empty())
    return;

  auto CMD = Compiler + " " + SourceFile.Opt + " -emit-ast -o " + Path +
             " -c " + SourceFile.Path;
  if (system(CMD.c_str()) || !fs::exists(Path))
    return;

  Ptr = clang::ASTUnit::LoadFromASTFile(
      Path, clang::RawPCHContainerReader(), clang::ASTUnit::LoadASTOnly,
      clang::CompilerInstance::createDiagnostics(
          new clang::DiagnosticOptions()),
#if LLVM_VERSION_MAJOR*1000 + LLVM_VERSION_MINOR*10 + LLVM_VERSION_PATCH >= 17006
        clang::FileSystemOptions(),
        std::make_shared<clang::HeaderSearchOptions>());
#else
        clang::FileSystemOptions());
#endif
  if (!Ptr && fs::exists(Path))
    fs::remove(Path);
}

CompileHelper::ASTData::ASTData(ASTData &&Data) {

  Path = Data.Path;
  Ptr = std::move(Data.Ptr);
}

CompileHelper::ASTData::~ASTData() {

  Ptr.reset();
  if (fs::exists(Path))
    fs::remove(Path);
}

CompileHelper::IRData::IRData(const std::vector<SourceFileData> &SourceFiles)
    : Path(getUniqueFilePath(getTmpDirPath(), "link", "bc")) {

  std::vector<std::string> IRPaths;
  for (auto &SourceFile : SourceFiles) {
    auto IRPath = compile(SourceFile);
    if (IRPath.empty()) {
      IRPaths.clear();
      return;
    }

    IRPaths.emplace_back(IRPath);
  }
  if (IRPaths.empty())
    return;

  auto CMD = "llvm-link -o " + Path;
  for (auto &IRPath : IRPaths)
    CMD += " " + IRPath;
  if (system(CMD.c_str()) || !fs::exists(Path))
    return;

  for (auto &IRPath : IRPaths) {
    if (!fs::exists(IRPath))
      continue;
    fs::remove(IRPath);
  }

  Ctx = std::make_unique<llvm::LLVMContext>();
  if (!Ctx)
    return;

  llvm::SMDiagnostic SMDiag;
  Ptr = llvm::parseIRFile(Path, SMDiag, *Ctx);
  if (!Ptr && fs::exists(Path))
    fs::remove(Path);
}

CompileHelper::IRData::~IRData() {

  Ptr.reset();
  Ctx.reset();
  if (fs::exists(Path))
    fs::remove(Path);
}

std::string
CompileHelper::IRData::compile(const SourceFileData &SourceFile) const {

  auto Compiler = CompileHelper::getCompiler(SourceFile.Type);
  if (Compiler.empty())
    return "";

  auto Result = getUniqueFilePath(getTmpDirPath(), SourceFile.Name, "bc");
  auto CMD = Compiler + " " + SourceFile.Opt + " -emit-llvm -o " + Result +
             " -c " + SourceFile.Path;
  if (system(CMD.c_str()) || !fs::exists(Result))
    return "";

  return Result;
}

} // namespace ftg
