#ifndef FTG_TESTUTIL_CODECOMPILER_H
#define FTG_TESTUTIL_CODECOMPILER_H

#include "ftg/sourceloader/SourceLoader.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Module.h"
#include <string>
#include <vector>

namespace ftg {

class CompileHelper : public SourceLoader {

public:
  CompileHelper(std::string SrcDir = "");
  enum SourceType { SourceType_C, SourceType_CPP, SourceType_HEADER };

  static std::string getCompiler(SourceType Type);
  static std::string getFileExt(SourceType Type);

  std::unique_ptr<SourceCollection> load() override;

  ~CompileHelper();
  bool add(const std::string Code, const std::string Name,
           const std::string Opt, SourceType Type);
  bool add(const std::string Path, const std::string Opt, SourceType Type);
  bool compileAST();
  bool compileIR();

  std::vector<clang::ASTUnit *> getASTUnits();
  std::vector<std::unique_ptr<clang::ASTUnit>> takeASTUnits();
  llvm::Module *getLLVMModule();
  std::string getIRPath() const;

  void printASTUnits() const;
  void printLLVMModule() const;

private:
  std::string SrcDir;
  struct SourceFileData {
    std::string Name;
    std::string Path;
    std::string Opt;
    SourceType Type;
    bool Generated;

    SourceFileData(const std::string Name, const std::string Path,
                   const std::string Opt, SourceType Type);
    SourceFileData(const std::string Path, const std::string Opt,
                   SourceType Type);
  };

  struct ASTData {
    std::string Path;
    std::unique_ptr<clang::ASTUnit> Ptr;

    ASTData(const SourceFileData &SourceFile);
    ASTData(ASTData &&Data);
    ~ASTData();
  };

  struct IRData {
    std::string Path;
    std::unique_ptr<llvm::Module> Ptr;
    std::unique_ptr<llvm::LLVMContext> Ctx;

    IRData(const std::vector<SourceFileData> &SourceFiles);
    ~IRData();
    std::string compile(const SourceFileData &SourceFile) const;
  };

  std::vector<SourceFileData> SourceFiles;
  std::vector<ASTData> ASTFiles;
  std::unique_ptr<IRData> IRFile;
};

} // namespace ftg

#endif // FTG_TESTUTIL_CODECOMPILER_H
