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

std::tuple<bool, std::string, std::string, std::string, std::string,
           std::string, std::string, std::string>
parseCommandLineOptions(int argc, const char **argv) {
  llvm::cl::ResetCommandLineParser();
  llvm::cl::opt<bool> Help("help", llvm::cl::desc("Show Options"));
  llvm::cl::opt<std::string> ProjectEntryPath(
      "entry", llvm::cl::desc("Project Entry Path"),
      llvm::cl::value_desc("filepath"));
  llvm::cl::opt<std::string> BinaryName("name", llvm::cl::desc("Binary Name"),
                                        llvm::cl::desc("name"));
  llvm::cl::opt<std::string> LibraryAPIPath("public",
                                            llvm::cl::desc("Public API Path"),
                                            llvm::cl::value_desc("filepath"));
  llvm::cl::opt<std::string> TargetPath(
      "target", llvm::cl::desc("Target Analyzer Report Directory Path"),
      llvm::cl::value_desc("filepath"));
  llvm::cl::opt<std::string> ExternPath(
      "extern", llvm::cl::desc("External Library Report Directory Path"),
      llvm::cl::value_desc("filepath"));
  llvm::cl::opt<std::string> UTType("ut", llvm::cl::desc("UT Type"),
                                    llvm::cl::value_desc("string"));
  llvm::cl::opt<std::string> OutputPath("out",
                                        llvm::cl::desc("An output file path"),
                                        llvm::cl::value_desc("filepath"));
  llvm::cl::ParseCommandLineOptions(argc, argv, "UT Analyzer\n");
  return std::make_tuple(bool(Help), std::string(ProjectEntryPath),
                         std::string(BinaryName), std::string(LibraryAPIPath),
                         std::string(TargetPath), std::string(ExternPath),
                         std::string(UTType), std::string(OutputPath));
}

int main(int argc, const char **argv) {

  increaseStackSize();

  bool Help;
  std::string ProjectEntryPath, BinaryName, LibraryAPIPath, TargetPath,
      ExternPath, UTType, OutputPath;
  std::tie(Help, ProjectEntryPath, BinaryName, LibraryAPIPath, TargetPath,
           ExternPath, UTType, OutputPath) =
      parseCommandLineOptions(argc, argv);

  if (Help || ProjectEntryPath.empty() || BinaryName.empty() ||
      LibraryAPIPath.empty() || UTType.empty() || OutputPath.empty()) {
    llvm::cl::PrintHelpMessage();
    return 0;
  }

  try {
    UTAnalyzer Analyzer(
        std::make_shared<BuildDBLoader>(ProjectEntryPath, BinaryName),
        std::make_shared<APIJsonLoader>(LibraryAPIPath), TargetPath, ExternPath,
        UTType);

    if (!Analyzer.analyze())
      return 1;

    if (!Analyzer.dump(OutputPath))
      return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}
