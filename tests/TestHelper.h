#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include "ftg/astirmap/ASTIRMap.h"
#include "ftg/generation/Generator.h"
#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/utanalysis/UTAnalyzer.h"
#include "ftg/utanalysis/UTLoader.h"
#include "testutil/CompileHelper.h"
#include "testutil/IRAccessHelper.h"
#include "testutil/TestUtil.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include <experimental/filesystem>
#include <gtest/gtest.h>

namespace ftg {

std::unique_ptr<Enum> createEnum(std::string TypeName, clang::ASTContext &Ctx);
std::string getBaseDirPath();
std::string getProjectBaseDirPath(std::string ProjectName);
std::string getProjectOutDirPath(std::string ProjectName);
std::string getPublicAPIPath(std::string ProjectName);
std::string getPreSourcePath(std::string ProjectName);
std::string getTargetSourcePath(std::string ProjectName);
std::string getTargetReportPath(std::string ProjectName);
std::string getTargetReportDirPath(std::string ProjectName);
std::string getUTSourcePath(std::string ProjectName);
std::string getUTReportPath(std::string ProjectName);
/// Verifies file result using ApprovalTests.
void verifyFile(const std::string &FilePath);
/// Verifies multiple file results using ApprovalTests.
void verifyFiles(const std::vector<std::string> &FilePaths);

struct InstIndex {
  std::string FuncName;
  unsigned BIdx;
  unsigned IIdx;
  int OIdx;
};

class TATestHelper {

public:
  TATestHelper(std::shared_ptr<CompileHelper> Compile);

  bool addExternal(std::shared_ptr<TATestHelper> TA);
  bool analyze();
  bool dumpReport(std::string Path);

  const llvm::Module *getModule() const;
  TargetLib &getTargetLib();
  TargetLibAnalyzer &getAnalyzer();

private:
  std::shared_ptr<CompileHelper> Compile;
  std::vector<std::shared_ptr<TATestHelper>> Externals;
  std::unique_ptr<TargetLibAnalyzer> Analyzer;
};

class UATestHelper {

public:
  UATestHelper(std::shared_ptr<CompileHelper> Compile = nullptr);

  void addTA(std::shared_ptr<TATestHelper> TA);
  bool analyze(std::shared_ptr<APILoader> AL, std::string UTType = "tct");
  bool analyze(std::string TargetReportDirPath, std::shared_ptr<APILoader> AL,
               std::string UTType);
  bool analyze(std::shared_ptr<UTLoader> Loader, std::string UTType = "tct");
  bool dumpReport(std::string Path);

  std::vector<std::shared_ptr<TATestHelper>> getTATestHelpers();

private:
  std::shared_ptr<CompileHelper> Compile;
  std::unique_ptr<SourceCollection> SC;
  std::vector<std::shared_ptr<TATestHelper>> TAs;
  std::unique_ptr<UTAnalyzer> Analyzer;
};

class GenTestHelper {

public:
  GenTestHelper(std::shared_ptr<UATestHelper> UA);
  ~GenTestHelper();

  bool generate(std::string BaseDir, std::string OutputDir,
                std::vector<std::string> APINames);

  const Generator &getGenerator() const;
  /// Verify generated sources using ApprovalTest.
  /// This function should be called after generate finished.
  void verifyGenerated(std::string ProjectName) const;

private:
  std::shared_ptr<UATestHelper> UA;
  Generator FuzzGenerator;
  std::string ProjectName;
  std::string TargetDirPath;
  std::string UTReportPath;
  const std::vector<std::string> FuzzFiles = {"fuzz_entry.cc",
                                              "FuzzArgsProfile.proto"};

  std::set<std::string> getPublicAPIs() const;

  bool storeTargetAnalysisReport(std::string DirPath);
};

class TestHelperFactory {

public:
  std::shared_ptr<CompileHelper>
  createCompileHelper(std::string Code, std::string Name, std::string Opt,
                      CompileHelper::SourceType Type) const;
  std::shared_ptr<CompileHelper>
  createCompileHelper(std::string SrcDir, std::vector<std::string> Paths,
                      std::string Opt, CompileHelper::SourceType Type) const;
  std::shared_ptr<TATestHelper>
  createTATestHelper(std::string Code, std::string Name,
                     CompileHelper::SourceType Type) const;
  std::shared_ptr<TATestHelper>
  createTATestHelper(std::string SrcDir, std::vector<std::string> SrcPaths,
                     CompileHelper::SourceType Type) const;
  std::shared_ptr<UATestHelper>
  createUATestHelper(std::string Code, std::string Name,
                     CompileHelper::SourceType Type) const;
  std::shared_ptr<UATestHelper>
  createUATestHelper(std::string SrcDir, std::vector<std::string> SrcPaths,
                     CompileHelper::SourceType Type) const;
};

class TestBase : public ::testing::Test {

protected:
  std::shared_ptr<CompileHelper> CH;
  std::unique_ptr<ASTIRMap> AIMap;
  std::shared_ptr<IRAccessHelper> IRAH;
  std::unique_ptr<SourceCollection> SC;

  bool load(const std::string &CODE, std::string Opt,
            CompileHelper::SourceType Type);
  bool loadC(const std::string &CODE, std::string Opt = "-O0 -g");
  bool loadCPP(const std::string &CODE, std::string Opt = "-O0 -g");
};

} // namespace ftg

#endif
/* TEST_HELPER_H */
