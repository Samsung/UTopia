//===-- CompileDB.cpp - Compile DB ----------------------------------------===//
//
// Class representation of compiles.json in build db
//
//===----------------------------------------------------------------------===//

#include "ftg/sourceloader/BuildDB.h"

#include "ftg/utils/FileUtil.h"

#include <experimental/filesystem>
#include <iostream>

namespace fs = std::experimental::filesystem;

using namespace ftg;

std::unique_ptr<CompileDB> CompileDB::fromJson(std::string JsonPath) {
  if (!fs::is_regular_file(JsonPath))
    return nullptr;

  auto JsonRoot = util::parseJsonFileToJsonValue(JsonPath.c_str());
  std::vector<CompileInfo> Compiles;
  try {
    for (auto CompileInfoValue : JsonRoot) {
      Compiles.push_back(CompileInfo{CompileInfoValue["ast_command"].asString(),
                                     CompileInfoValue["ast_file"].asString(),
                                     CompileInfoValue["bc_command"].asString(),
                                     CompileInfoValue["bc_file"].asString(),
                                     CompileInfoValue["directory"].asString(),
                                     CompileInfoValue["out_command"].asString(),
                                     CompileInfoValue["out_file"].asString(),
                                     CompileInfoValue["src_file"].asString()});
    }
    return std::make_unique<CompileDB>(Compiles);
  } catch (...) {
    return nullptr;
  }
}
