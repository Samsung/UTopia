#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/sourceloader/BuildDBLoader.h"
#include "ftg/utanalysis/UTAnalyzer.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include <sys/resource.h>

using namespace ftg;

void increaseStackSize() {

  const unsigned long long MAX = 100 * 1024 * 1024;
  struct rlimit Limit;

  if (getrlimit(RLIMIT_STACK, &Limit) == 0) {
    if (MAX < Limit.rlim_cur) {
      llvm::outs() << "[I] Default stack size is enough.\n";
      return;
    }
  }

  Limit.rlim_cur = MAX;
  Limit.rlim_max = MAX;
  if (setrlimit(RLIMIT_STACK, &Limit) != 0) {
    llvm::outs() << "[W] Unable to increase stack size.\n";
    return;
  }

  if (getrlimit(RLIMIT_STACK, &Limit) == 0) {
    llvm::outs() << "[I] Stack size increased to "
                 << Limit.rlim_cur / 1024 / 1024 << " MB.\n";
  }
}

int main(int argc, const char **argv) {
  llvm::cl::getRegisteredOptions().clear();
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
  llvm::cl::opt<std::string> BuildDBPath(
      "db", llvm::cl::desc("<Required> Path of build db json file"),
      llvm::cl::value_desc("filepath"), llvm::cl::Required);
  llvm::cl::opt<std::string> LibraryAPIPath(
      "public", llvm::cl::desc("<Required> PublicAPI.json path"),
      llvm::cl::value_desc("filepath"), llvm::cl::Required);
  llvm::cl::opt<std::string> TargetReportDir(
      "target", llvm::cl::desc("Target Analyzer report directory path"),
      llvm::cl::value_desc("dir_path"));
  llvm::cl::opt<std::string> ExternReportDir(
      "extern", llvm::cl::desc("External library report directory path"),
      llvm::cl::value_desc("dir_path"));
  llvm::cl::opt<std::string> UTType("ut", llvm::cl::desc("<Required> UT type"),
                                    llvm::cl::value_desc("string"),
                                    llvm::cl::Required);
  llvm::cl::opt<std::string> OutputPath(
      "out", llvm::cl::desc("<Required> Output file path"),
      llvm::cl::value_desc("filepath"), llvm::cl::Required);
  llvm::cl::ParseCommandLineOptions(argc, argv, "UT Analyzer\n");

  increaseStackSize();
  try {
    UTAnalyzer Analyzer(std::make_shared<BuildDBLoader>(BuildDBPath),
                        std::make_shared<APIJsonLoader>(LibraryAPIPath),
                        TargetReportDir, ExternReportDir, UTType);

    if (!Analyzer.analyze())
      return 1;

    if (!Analyzer.dump(OutputPath))
      return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}
