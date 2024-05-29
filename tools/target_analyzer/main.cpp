#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/sourceloader/BuildDBLoader.h"
#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/utils/FileUtil.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;
using namespace ftg;

int main(int argc, const char **argv) {
  cl::ResetCommandLineParser();
  // FIXME: Workaround to avoid llvm inconsistency error
  // cl::opt<bool> OptHelp("help", cl::desc("Display available options"),
  //                       cl::ValueDisallowed, cl::callback([](const bool &) {
  //                         cl::PrintHelpMessage();
  //                         exit(0);
  //                       }));
  // cl::opt<bool> OptVersion("version", cl::desc("Display version"),
  //                          cl::ValueDisallowed, cl::callback([](const bool &) {
  //                            llvm::outs() << "version: " << UTOPIA_VERSION << "\n";
  //                            exit(0);
  //                          }));
  cl::opt<std::string> OutFilePath("out",
                                   cl::desc("<Required> Output filepath"),
                                   cl::value_desc("filepath"), cl::Required);
  cl::opt<std::string> BuildDBPath(
      "db", cl::desc("<Required> Path of build db json file"),
      cl::value_desc("filepath"), cl::Required);
  cl::opt<std::string> ExternReportDir(
      "extern", cl::desc("Pre-Analyzed TargetLib analysis results directory"),
      cl::value_desc("dir_path"));
  cl::opt<std::string> PublicAPIJsonPath(
      "public", cl::desc("<Required> PublicAPI.json filepath to analyze"),
      cl::value_desc("filepath"), cl::Required);

  cl::ParseCommandLineOptions(argc, argv, "Target Analyzer\n");

  std::shared_ptr<APILoader> AL;
  AL = std::make_shared<APIJsonLoader>(PublicAPIJsonPath);

  try {
    TargetLibAnalyzer Analyzer(std::make_shared<BuildDBLoader>(BuildDBPath), AL,
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
