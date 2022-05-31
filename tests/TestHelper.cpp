#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/inputanalysis/DefAnalyzer.h"
#include "ftg/utanalysis/TargetAPIFinder.h"

#include "ftg/utils/FileUtil.h"
#include "ftg/utils/PublicAPI.h"
#include "testutil/APIManualLoader.h"

#include "clang/Frontend/ASTUnit.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Serialization/PCHContainerOperations.h"

#include "approvals/ApprovalTests.hpp"
#include "llvm/IRReader/IRReader.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

std::string getBaseDirPath() {

  return fs::absolute(fs::path("tests") / fs::path("resources")).string();
}

std::string getProjectBaseDirPath(std::string ProjectName) {

  return (fs::path(getBaseDirPath()) / fs::path("SOURCES") /
          fs::path(ProjectName))
      .string();
}

std::string getProjectOutDirPath(std::string ProjectName) {

  return (fs::path(getBaseDirPath()) / fs::path("out") / fs::path(ProjectName))
      .string();
}

std::string getPublicAPIPath(std::string ProjectName) {

  return (fs::path(getProjectBaseDirPath(ProjectName)) / fs::path("api.json"))
      .string();
}

std::string getPreSourcePath(std::string ProjectName) {

  return (fs::path(getProjectBaseDirPath(ProjectName)) / fs::path("lib") /
          fs::path("pre.c"))
      .string();
}

std::string getTargetSourcePath(std::string ProjectName) {

  return (fs::path(getProjectBaseDirPath(ProjectName)) / fs::path("lib") /
          fs::path("lib.c"))
      .string();
}

std::string getTargetReportPath(std::string ProjectName) {

  return (fs::path(getProjectOutDirPath(ProjectName)) / fs::path("ta") /
          fs::path("ta_report.json"))
      .string();
}

std::string getTargetReportDirPath(std::string ProjectName) {

  return util::getParentPath(getTargetReportPath(ProjectName));
}

std::string getUTSourcePath(std::string ProjectName) {

  return (fs::path(getProjectBaseDirPath(ProjectName)) / fs::path("tct") /
          fs::path("test.c"))
      .string();
}

std::string getUTReportPath(std::string ProjectName) {

  return (fs::path(getProjectOutDirPath(ProjectName)) /
          fs::path("ua_report.json"));
}

void verifyFile(const std::string &FilePath) {
  ApprovalTests::Approvals::verifyExistingFile(
      FilePath,
      ApprovalTests::Options(ApprovalTests::AutoApproveIfMissingReporter()));
}

void verifyFiles(const std::vector<std::string> &FilePaths) {
  ApprovalTests::ExceptionCollector Exceptions;
  for (auto FilePath : FilePaths) {
    auto Section = ApprovalTests::NamerFactory::appendToOutputFilename(
        fs::path(FilePath).stem().string());
    Exceptions.gather([&]() { verifyFile(FilePath); });
  }
  Exceptions.release();
}

TATestHelper::TATestHelper(std::shared_ptr<CompileHelper> Compile)
    : Compile(Compile) {}

bool TATestHelper::addExternal(std::shared_ptr<TATestHelper> TA) {

  if (!TA)
    return false;
  Externals.emplace_back(TA);
  return true;
}

bool TATestHelper::analyze() {
  if (!Compile)
    return false;

  auto ExternalReportDir = getUniqueFilePath(getTmpDirPath(), "target", "");
  if (!fs::create_directories(ExternalReportDir))
    return false;

  for (auto &External : Externals) {
    if (!External)
      continue;

    auto ExternalReportPath =
        getUniqueFilePath(ExternalReportDir, "external", "json");
    if (!External->dumpReport(ExternalReportPath))
      continue;
  }

  Analyzer = std::make_unique<TargetLibAnalyzer>(
      Compile, std::make_shared<APIManualLoader>(), ExternalReportDir);
  if (!Analyzer)
    return false;

  fs::remove_all(ExternalReportDir);
  return Analyzer->analyze();
}

bool TATestHelper::dumpReport(std::string Path) {
  if (!Analyzer)
    return false;
  return Analyzer->dump(Path);
}

const llvm::Module *TATestHelper::getModule() const {
  if (Compile && Compile->getLLVMModule())
    return Compile->getLLVMModule();

  if (!Analyzer)
    return nullptr;

  auto *SC = Analyzer->getSourceCollection();
  if (!SC)
    return nullptr;

  return &SC->getLLVMModule();
}

TargetLib &TATestHelper::getTargetLib() { return getAnalyzer().getTargetLib(); }

TargetLibAnalyzer &TATestHelper::getAnalyzer() {
  assert(Analyzer && "Unexpected Program State");
  return *Analyzer;
}

UATestHelper::UATestHelper(std::shared_ptr<CompileHelper> Compile)
    : Compile(Compile) {}

void UATestHelper::addTA(std::shared_ptr<TATestHelper> TA) {
  TAs.push_back(TA);
}

bool UATestHelper::analyze(std::shared_ptr<APILoader> AL) {
  auto TargetDirPath = getUniqueFilePath(getTmpDirPath(), "target", "");
  if (!fs::create_directories(TargetDirPath))
    return false;

  unsigned Cnt = 0;
  for (auto &TA : TAs) {
    if (!TA)
      continue;
    auto ReportPath = getUniqueFilePath(
        TargetDirPath, "target" + std::to_string(Cnt++), "json");
    if (!TA->dumpReport(ReportPath))
      return false;
  }

  std::vector<std::string> ReportPaths = {TargetDirPath};
  auto Loader = std::make_shared<UTLoader>(Compile, AL, ReportPaths);
  if (!Loader)
    return false;

  if (!fs::remove_all(TargetDirPath))
    return false;

  return analyze(Loader);
}

bool UATestHelper::analyze(std::string TargetReportDirPath,
                           std::shared_ptr<APILoader> AL, std::string UTType) {
  std::vector<std::string> ReportPaths = {TargetReportDirPath};
  auto Loader = std::make_shared<UTLoader>(Compile, AL, ReportPaths);
  return analyze(Loader, UTType);
}

bool UATestHelper::analyze(std::shared_ptr<UTLoader> Loader,
                           std::string UTType) {
  Analyzer = std::make_unique<UTAnalyzer>(Loader, UTType);
  if (!Analyzer)
    return false;

  return Analyzer->analyze();
}

bool UATestHelper::dumpReport(std::string Path) {
  if (!Analyzer)
    return false;

  return Analyzer->dump(Path);
}

std::vector<std::shared_ptr<TATestHelper>> UATestHelper::getTATestHelpers() {
  return TAs;
}

GenTestHelper::GenTestHelper(std::shared_ptr<UATestHelper> UA) : UA(UA) {}

GenTestHelper::~GenTestHelper() {

  if (fs::exists(TargetDirPath))
    fs::remove_all(TargetDirPath);
  if (fs::exists(UTReportPath))
    fs::remove(UTReportPath);
}

bool GenTestHelper::generate(std::string BaseDir, std::string OutputDir,
                             std::vector<std::string> APINames) {

  if (!UA)
    return false;

  TargetDirPath = getUniqueFilePath(getTmpDirPath(), "target", "");
  if (!storeTargetAnalysisReport(TargetDirPath))
    return false;

  UTReportPath = getUniqueFilePath(getTmpDirPath(), "ut", "json");
  if (!UA->dumpReport(UTReportPath))
    return false;

  std::set<std::string> APINameSet(APINames.begin(), APINames.end());

  if (fs::exists(OutputDir)) {
    if (!fs::remove_all(OutputDir))
      return false;
  }
  if (!fs::create_directories(OutputDir))
    return false;

  FuzzGenerator.generate(BaseDir, APINameSet, TargetDirPath, UTReportPath,
                         OutputDir);

  return true;
}

const Generator &GenTestHelper::getGenerator() const { return FuzzGenerator; }

bool GenTestHelper::generateDirectories() const {

  auto ProjectOutDirPath = getProjectOutDirPath(ProjectName);
  if (!fs::exists(ProjectOutDirPath)) {
    if (!fs::create_directories(ProjectOutDirPath))
      return false;
  }

  return true;
}

std::set<std::string> GenTestHelper::getPublicAPIs() const {

  PublicAPI Parser;
  Parser.loadJson(getPublicAPIPath(ProjectName));
  return Parser.getPublicAPIList();
}

bool GenTestHelper::storeTargetAnalysisReport(std::string DirPath) {

  if (!UA)
    return false;
  if (fs::exists(DirPath))
    return false;
  if (!fs::create_directories(DirPath))
    return false;

  unsigned cnt = 0;
  for (auto &TA : UA->getTATestHelpers()) {
    std::string Path = (fs::path(DirPath) /
                        fs::path(std::to_string(cnt++) + ".json").string());
    if (!TA->dumpReport(Path))
      return false;
  }

  return true;
}

void GenTestHelper::verifyGenerated(std::string ProjectName) const {
  ApprovalTests::ExceptionCollector Exceptions;
  auto &Reporter = FuzzGenerator.getFuzzGenReporter();
  auto UTReportMap = Reporter.getUTReportMap();
  for (auto UTReportPair : UTReportMap) {
    auto &UT = UTReportPair.first;
    auto &UTReport = UTReportPair.second;
    if (UTReport->Status != FUZZABLE_SRC_GENERATED)
      continue;
    auto OutFiles = FuzzFiles;
    OutFiles.push_back(UTReport->UTFileName);
    for (auto OutFile : OutFiles) {
      auto OutFilePath = fs::path(getProjectOutDirPath(ProjectName)) / UT /
                         UTReport->UTDir / OutFile;
      auto Section = ApprovalTests::NamerFactory::appendToOutputFilename(
          UT + "." + OutFilePath.stem().string());
      Exceptions.gather([&]() { verifyFile(OutFilePath.string()); });
    }
  }
  Exceptions.release();
}

std::shared_ptr<CompileHelper>
TestHelperFactory::createCompileHelper(std::string Code, std::string Name,
                                       std::string Opt,
                                       CompileHelper::SourceType Type) const {

  auto Compile = std::make_shared<CompileHelper>();
  if (!Compile)
    return nullptr;
  if (!Compile->add(Code, Name, Opt, Type))
    return nullptr;
  if (!Compile->compileAST())
    return nullptr;
  if (!Compile->compileIR())
    return nullptr;
  return Compile;
}

std::shared_ptr<CompileHelper> TestHelperFactory::createCompileHelper(
    std::string SrcDir, std::vector<std::string> SrcPaths, std::string Opt,
    CompileHelper::SourceType Type) const {
  auto Compile = std::make_shared<CompileHelper>(SrcDir);
  if (!Compile)
    return nullptr;
  for (const auto &Path : SrcPaths)
    if (!Compile->add(Path, Opt, Type))
      return nullptr;
  if (!Compile->compileAST())
    return nullptr;
  if (!Compile->compileIR())
    return nullptr;
  return Compile;
}

std::shared_ptr<TATestHelper>
TestHelperFactory::createTATestHelper(std::string Code, std::string Name,
                                      CompileHelper::SourceType Type) const {

  auto Compile = createCompileHelper(Code, Name, "-O0 -fPIC", Type);
  if (!Compile)
    return nullptr;

  return std::make_shared<TATestHelper>(Compile);
}

std::shared_ptr<TATestHelper>
TestHelperFactory::createTATestHelper(std::string SrcDir,
                                      std::vector<std::string> SrcPaths,
                                      CompileHelper::SourceType Type) const {

  auto Compile = createCompileHelper(SrcDir, SrcPaths, "-O0", Type);
  if (!Compile)
    return nullptr;

  return std::make_shared<TATestHelper>(Compile);
}

std::shared_ptr<UATestHelper>
TestHelperFactory::createUATestHelper(std::string Code, std::string Name,
                                      CompileHelper::SourceType Type) const {

  auto Compile = createCompileHelper(Code, Name, "-O0 -g", Type);
  if (!Compile)
    return nullptr;

  return std::make_shared<UATestHelper>(Compile);
}

std::shared_ptr<UATestHelper>
TestHelperFactory::createUATestHelper(std::string SrcDir,
                                      std::vector<std::string> SrcPaths,
                                      CompileHelper::SourceType Type) const {
  auto Compile = createCompileHelper(SrcDir, SrcPaths, "-O0 -g", Type);
  if (!Compile)
    return nullptr;

  return std::make_shared<UATestHelper>(Compile);
}

std::string TestHelperFactory::createPublicAPIJson(
    std::vector<std::string> APINames) const {

  std::string Json = "{ \"test\" : [ ";
  for (int S = 0, E = (int)APINames.size(); S < E; ++S) {
    Json += "\"" + APINames[S] + "\"";
    if (S < E - 1)
      Json += ", ";
  }
  Json += " ] }";

  auto JsonPath = getUniqueFilePath(getTmpDirPath(), "api", "json");
  if (!util::saveFile(JsonPath.c_str(), Json.c_str()))
    return "";
  return JsonPath;
}

bool TestBase::load(const std::string &CODE, std::string Name, std::string Opt,
                    CompileHelper::SourceType Type) {

  CH = TestHelperFactory().createCompileHelper(CODE, Name, Opt, Type);
  if (!CH)
    return false;

  const llvm::Module &M(*CH->getLLVMModule());
  IRAH = std::make_shared<IRAccessHelper>(M);
  if (!IRAH)
    return false;

  return true;
}

bool TestBase::loadC(const std::string &CODE, std::string Name,
                     std::string Opt) {
  return load(CODE, Name, Opt, CompileHelper::SourceType_C);
}

bool TestBase::loadCPP(const std::string &CODE, std::string Name,
                       std::string Opt) {
  return load(CODE, Name, Opt, CompileHelper::SourceType_CPP);
}
}; // namespace ftg
