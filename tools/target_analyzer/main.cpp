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
  cl::opt<bool> OptionHelp("help", cl::desc("Display available options"));
  cl::opt<std::string> OptionOutFilePath(
      "out", cl::desc("Output filepath <needed>"), cl::value_desc("filepath"));
  cl::opt<std::string> OptionEntryJsonPath(
      "entry", cl::desc("project_entry.json"), cl::value_desc("filepath"));
  cl::opt<std::string> OptionLibraryName("name", cl::desc("library name"),
                                         cl::value_desc("name"));
  cl::opt<std::string> OptionAST("ast", cl::desc("AST directory to analyze"),
                                 cl::value_desc("dir"));
  cl::opt<std::string> OptionLLVMIR("ir",
                                    cl::desc("LLVM IR filepath to analyze"),
                                    cl::value_desc("filepath"));
  cl::opt<std::string> OptionExternLibDir(
      "extern", cl::desc("Pre-Analyzed TargetLib Analysis Results directory"),
      cl::value_desc("dir"));
  cl::opt<std::string> OptionPublicAPI(
      "public", cl::desc("PublicAPI json filepath to analyze"),
      cl::value_desc("filepath"));
  cl::opt<std::string> OptionExportsNDeps(
      "exports-deps", cl::desc("exports_and_dependencies.json filepath"),
      cl::value_desc("filepath"));

  cl::ParseCommandLineOptions(argc, argv, "Target Analyzer\n");

  std::string ProjectEntryPath = OptionEntryJsonPath;
  std::string LibraryName = OptionLibraryName;
  std::string ExternLibDir = OptionExternLibDir;
  std::string OutFilePath = OptionOutFilePath;
  std::string PublicAPIJsonPath = OptionPublicAPI;
  std::string ExportsNDepsJsonPath = OptionExportsNDeps;

  if (ExportsNDepsJsonPath.empty() && PublicAPIJsonPath.empty())
    return 1;

  std::shared_ptr<APILoader> AL;
  if (!ExportsNDepsJsonPath.empty())
    AL = std::make_shared<APIExportsNDepsJsonLoader>(ExportsNDepsJsonPath,
                                                     LibraryName);
  else
    AL = std::make_shared<APIJsonLoader>(PublicAPIJsonPath);

  TargetLibAnalyzer Analyzer(
      std::make_shared<BuildDBLoader>(ProjectEntryPath, LibraryName), AL,
      ExternLibDir);
  if (!Analyzer.analyze())
    return 1;

  if (!Analyzer.dump(OutFilePath))
    return 1;

  return 0;
}
