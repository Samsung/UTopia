#include "ftg/utils/BuildDBParser.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include "llvm/Support/raw_ostream.h"
#include <assert.h>
#include <iostream>
namespace Json {

class Value;

};

namespace ftg {

std::string getTCTProjectName(std::string rawProjectName) {
  if (rawProjectName.find("core-") == 0) {
    rawProjectName = rawProjectName.substr(5, rawProjectName.length());
  }
  return rawProjectName.substr(0, rawProjectName.rfind("-tests-"));
}

std::string getStrippedLibraryName(std::string rawLibraryName) {
  std::string ret = util::regex(rawLibraryName, "(.+)\\.so\\.", 1);
  assert(!ret.empty() && "Failed to get LibraryName!");
  return ret;
}

static const std::string PROJECT_ENTRY_FILENAME = "project_entry.json";

BuildDBParser::BuildDBParser(std::string jsonPath) {
  // load project_entry.json
  try {
    Json::Value projectEntryJson =
        util::parseJsonFileToJsonValue(jsonPath.c_str());
    assert(!projectEntryJson.isNull() && "parsing project entry json failed");

    // set project build information
    m_projectName = projectEntryJson["project_name"].asString();
    m_BuildDir = projectEntryJson["project_dir"].asString();

    // set project source directory
    m_SrcDir = m_BuildDir;
    size_t pos = m_SrcDir.rfind("BUILD" + PATH_SEPARATOR + m_projectName);
    if (pos != std::string::npos) {
      m_SrcDir = m_SrcDir.replace(pos, m_SrcDir.length(),
                                  "SOURCES" + PATH_SEPARATOR + m_projectName +
                                      PATH_SEPARATOR);
    } else {
      llvm::errs() << "[WARN] Unable to get project source path from project "
                      "build dir: "
                   << m_SrcDir << "\n";
    }

    // set binary build information
    for (std::string binaryName :
         projectEntryJson["binary_info"].getMemberNames()) {
      struct BuildResult buildResult;
      Json::Value binaryInfoJson = projectEntryJson["binary_info"][binaryName];
      buildResult.m_soFilePath = binaryInfoJson["out_file"].asString();
      buildResult.m_bcFilePath = binaryInfoJson["bc_file"].asString();
      BuildResults.insert(std::make_pair(binaryName, buildResult));
    }
  } catch (Json::Exception &e) {
    std::cout << "Json Exception : " << e.what() << std::endl;
    assert("BuildDBParser Fail");
  }
}

std::string BuildDBParser::getBCFilePath(std::string binaryName) {
  auto buildResult = BuildResults.find(binaryName);
  assert((buildResult != BuildResults.end()) &&
         "unable to find binary in project entry json file!");
  return buildResult->second.m_bcFilePath;
}

std::string BuildDBParser::getSoFilePath(std::string binaryName) {
  auto buildResult = BuildResults.find(binaryName);
  assert((buildResult != BuildResults.end()) &&
         "unable to find binary in project entry json file!");
  return buildResult->second.m_soFilePath;
}

CompileDBParser::CompileDBParser(std::string jsonDir, std::string binaryName) {
  try {
    std::string jsonPath =
        jsonDir + PATH_SEPARATOR + "compiles_v2_" + binaryName + ".json";
    Json::Value binaryCompileJson =
        util::parseJsonFileToJsonValue(jsonPath.c_str());

    for (Json::Value fileCompileJson : binaryCompileJson)
      m_astFilePaths.push_back(fileCompileJson["ast_file"].asString());
  } catch (Json::Exception &e) {
    std::cout << "Json Exception : " << e.what() << std::endl;
    assert("Can not find valid ast_file path");
  }
}

} // namespace ftg
