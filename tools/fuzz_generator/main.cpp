#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/generation/Generator.h"
#include "ftg/utils/BuildDBParser.h"
#include "ftg/utils/FileUtil.h"
#include "spdlog/spdlog.h"
#include <string>

using namespace ftg;

struct CMDOpts {
  std::string ProjectSrcPath;
  std::string TargetAnalysisResultPath;
  std::string UTAnalysisResultPath;
  std::string PublicAPIListPath;
  std::string OutputDirPath;
};

std::unique_ptr<CMDOpts> parseCommandLineArguments(int argc,
                                                   const char **argv) {
  if (argc != 6)
    throw std::invalid_argument("Command Line Options Should have 5 inputs");

  auto Result = std::make_unique<CMDOpts>();
  Result->ProjectSrcPath = std::string(argv[1]);
  Result->TargetAnalysisResultPath = std::string(argv[2]);
  Result->UTAnalysisResultPath = std::string(argv[3]);
  Result->PublicAPIListPath = std::string(argv[4]);
  Result->OutputDirPath = std::string(argv[5]);
  return Result;
}

std::set<std::string> getPublicAPIList(std::string PublicAPIListPath) {
  auto APILoader = APIJsonLoader(PublicAPIListPath);
  auto PublicAPIs = APILoader.load();
  assert(!PublicAPIs.empty() && "Public API List is empty");
  return PublicAPIs;
}

int main(int argc, const char **argv) {
  // argv[1] : project source directory path
  // argv[2] : target analyzer result path
  // argv[3] : ut analyzer result path
  // argv[4] : public api list path
  // argv[5] : Output directory path

  std::unique_ptr<CMDOpts> Opts;
  try {
    Opts = parseCommandLineArguments(argc, argv);
  } catch (std::invalid_argument &E) {
    spdlog::error(E.what());
    return 1;
  }
  assert(Opts && "Unexpected Program State");

  auto SrcDir = Opts->ProjectSrcPath;
  auto PublicAPIList = getPublicAPIList(Opts->PublicAPIListPath);

  Generator Gen;
  try {
    Gen.generate(SrcDir, PublicAPIList, Opts->TargetAnalysisResultPath,
                 Opts->UTAnalysisResultPath, Opts->OutputDirPath);
  } catch (Json::RuntimeError &E) {
    return 1;
  }

  return 0;
}
