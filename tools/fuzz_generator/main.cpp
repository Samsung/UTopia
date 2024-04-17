#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/generation/Generator.h"
#include "ftg/utils/FileUtil.h"
#include "spdlog/spdlog.h"
#include "llvm/Support/CommandLine.h"
#include <string>

using namespace ftg;

std::set<std::string> getPublicAPIList(std::string PublicAPIListPath) {
  auto APILoader = APIJsonLoader(PublicAPIListPath);
  auto PublicAPIs = APILoader.load();
  assert(!PublicAPIs.empty() && "Public API List is empty");
  return PublicAPIs;
}

int main(int argc, const char **argv) {
  llvm::cl::ResetCommandLineParser();
  llvm::cl::opt<bool> OptHelp(
      "help", llvm::cl::desc("Display available options"),
      llvm::cl::ValueDisallowed, llvm::cl::callback([](const bool &) {
        llvm::cl::PrintHelpMessage();
        exit(0);
      }));
  llvm::cl::opt<bool> OptVersion(
      "version", llvm::cl::desc("Display version"), llvm::cl::ValueDisallowed,
      llvm::cl::callback([](const bool &) {
        llvm::outs() << "version: " << UTOPIA_VERSION << "\n";
        exit(0);
      }));
  llvm::cl::opt<std::string> SrcDir(
      "src",
      llvm::cl::desc(
          "<Required> Original source directory of the target project"),
      llvm::cl::value_desc("dir_path"), llvm::cl::Required);
  llvm::cl::opt<std::string> TargetReportDir(
      "target",
      llvm::cl::desc("<Required> Target Analyzer report directory path"),
      llvm::cl::value_desc("dir_path"), llvm::cl::Required);
  llvm::cl::opt<std::string> UTReportPath(
      "ut", llvm::cl::desc("<Required> UT Analyzer report path"),
      llvm::cl::value_desc("filepath"), llvm::cl::Required);
  llvm::cl::opt<std::string> LibraryAPIPath(
      "public", llvm::cl::desc("<Required> PublicAPI.json path"),
      llvm::cl::value_desc("filepath"), llvm::cl::Required);
  llvm::cl::opt<std::string> OutputPath(
      "out", llvm::cl::desc("<Required> Output directory path"),
      llvm::cl::value_desc("dir_path"), llvm::cl::Required);
  llvm::cl::ParseCommandLineOptions(argc, argv, "Fuzz Generator\n");

  auto PublicAPIList = getPublicAPIList(LibraryAPIPath);

  Generator Gen;
  try {
    Gen.generate(SrcDir, PublicAPIList, TargetReportDir, UTReportPath,
                 OutputPath);
  } catch (Json::RuntimeError &E) {
    return 1;
  }

  return 0;
}
