#include "ftg/apiloader/APIExportsNDepsJsonLoader.h"
#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/sourceloader/BuildDBLoader.h"
#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/utils/FileUtil.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace ftg;

int main(int argc, const char **argv) {
  cl::ResetCommandLineParser();
  cl::opt<bool> OptHelp("help", cl::desc("Display available options"),
                        cl::ValueDisallowed, cl::callback([](const bool &) {
                          cl::PrintHelpMessage();
                          exit(0);
                        }));
  cl::opt<bool> OptVersion("version", cl::desc("Display version"),
                           cl::ValueDisallowed, cl::callback([](const bool &) {
                             llvm::outs() << "version: " << FTG_VERSION << "\n";
                             exit(0);
                           }));
  cl::opt<std::string> OutFilePath("out",
                                   cl::desc("<Required> Output filepath"),
                                   cl::value_desc("filepath"), cl::Required);
  cl::opt<std::string> ProjectEntryPath(
      "entry", cl::desc("<Required> project_entry.json path"),
      cl::value_desc("filepath"), cl::Required);
  cl::opt<std::string> LibraryName("name", cl::desc("<Required> Library name"),
                                   cl::value_desc("name"), cl::Required);
  cl::opt<std::string> ExternReportDir(
      "extern", cl::desc("Pre-Analyzed TargetLib analysis results directory"),
      cl::value_desc("dir_path"));
  cl::opt<std::string> PublicAPIJsonPath(
      "public", cl::desc("PublicAPI.json filepath to analyze"),
      cl::value_desc("filepath"));
  cl::opt<std::string> ExportsNDepsJsonPath(
      "exports-deps", cl::desc("exports_and_dependencies.json filepath"),
      cl::value_desc("filepath"));

  cl::ParseCommandLineOptions(argc, argv, "Target Analyzer\n");

  if (ExportsNDepsJsonPath.empty() && PublicAPIJsonPath.empty()) {
    llvm::errs()
        << "PublicAPI.json or exports_and_dependencies.json must be provided\n";
    return 1;
  }

  std::shared_ptr<APILoader> AL;
  if (!ExportsNDepsJsonPath.empty())
    AL = std::make_shared<APIExportsNDepsJsonLoader>(ExportsNDepsJsonPath,
                                                     LibraryName);
  else
    AL = std::make_shared<APIJsonLoader>(PublicAPIJsonPath);

  try {
    TargetLibAnalyzer Analyzer(
        std::make_shared<BuildDBLoader>(ProjectEntryPath, LibraryName), AL,
        ExternReportDir);
    if (!Analyzer.analyze())
      return 1;

    if (!Analyzer.dump(OutFilePath))
      return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}
