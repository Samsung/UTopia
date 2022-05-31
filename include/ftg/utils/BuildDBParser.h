#ifndef FTG_UTILS_BUILDDBPARSER_H
#define FTG_UTILS_BUILDDBPARSER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ftg {

// project name format : core-{PROJECT_NAME}-tests-x.x
// for getting PROJECT_NAME using replace string funciton.
std::string getTCTProjectName(std::string rawProjectName);

// (ex) libaccount-service-x.x.x (raw) -> libaccount-service (stripped)
std::string getStrippedLibraryName(std::string rawLibraryName);

/// This class gets information from project build result json
/// (project_entry.json). It provides various name and path information for one
/// project and binaries.
class BuildDBParser {
public:
  BuildDBParser(std::string jsonPath);

  std::string getProjectName() { return m_projectName; }

  std::string getBuildDir() { return m_BuildDir; }

  std::string getSrcDir() { return m_SrcDir; }

  std::string getBCFilePath(std::string binaryName);

  std::string getSoFilePath(std::string binaryName);

private:
  struct BuildResult {
    std::string m_bcFilePath; // linked bc file
    std::string m_soFilePath; // linked so file
  };

  std::string m_projectName;

  // project build directory
  std::string m_BuildDir;

  // original project source directory
  std::string m_SrcDir;

  // binary build results in the project
  std::map<std::string, BuildResult> BuildResults;
};

/// This class gets information from binary compile result json
/// (compiles_v2_{binaryName}.json). It provides AST filepath information per
/// source in one binary.
class CompileDBParser {
public:
  CompileDBParser(std::string jsonDir, std::string binaryName);

  std::vector<std::string> &getASTFilePaths() { return m_astFilePaths; }

private:
  std::vector<std::string> m_astFilePaths;
};

} // namespace ftg

#endif // FTG_UTILS_BUILDDBPARSER_H
