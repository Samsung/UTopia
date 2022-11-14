//===-- BuildDB.cpp - Implementation of BuildDB ---------------------------===//

#include "ftg/sourceloader/BuildDB.h"
#include "ftg/utils/FileUtil.h"

#include <experimental/filesystem>
#include <utility>

namespace fs = std::experimental::filesystem;
using namespace ftg;

BuildDB::BuildDB(std::string BCFile, std::vector<std::string> ASTFiles,
                 std::string ProjectDir)
    : BCPath(std::move(BCFile)), ASTPaths(std::move(ASTFiles)),
      ProjectDir(std::move(ProjectDir)) {}

std::unique_ptr<BuildDB> BuildDB::fromJson(const std::string &Path) {
  if (!fs::is_regular_file(Path))
    return nullptr;

  try {
    auto JsonRoot = util::parseJsonFileToJsonValue(Path.c_str());
    std::vector<std::string> ASTFiles;
    for (auto AST : JsonRoot["ast"]) {
      ASTFiles.emplace_back(AST.asString());
    }
    return std::make_unique<BuildDB>(JsonRoot["bc"].asString(), ASTFiles,
                                     JsonRoot["project_dir"].asString());
  } catch (...) {
    return nullptr;
  }
}

std::string BuildDB::getBCPath() const { return BCPath; }
std::vector<std::string> BuildDB::getASTPaths() const { return ASTPaths; }
std::string BuildDB::getProjectDir() const { return ProjectDir; }
