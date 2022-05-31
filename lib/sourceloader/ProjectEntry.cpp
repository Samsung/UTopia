//===-- ProjectEntry.cpp - Project Entry ----------------------------------===//
//
// Class representation of project_entry.json in build db
//
//===----------------------------------------------------------------------===//

#include "ftg/sourceloader/BuildDB.h"

#include "ftg/utils/FileUtil.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace ftg;

std::unique_ptr<ProjectEntry> ProjectEntry::fromJson(std::string JsonPath) {
  if (!fs::is_regular_file(JsonPath))
    return nullptr;
  auto JsonRoot = util::parseJsonFileToJsonValue(JsonPath.c_str());
  std::map<std::string, BCInfo> BinaryInfo;
  try {
    for (auto BinaryName : JsonRoot["binary_info"].getMemberNames()) {
      auto BCInfoValue = JsonRoot["binary_info"][BinaryName];
      BinaryInfo[BinaryName] = BCInfo{BCInfoValue["bc_command"].asString(),
                                      BCInfoValue["bc_file"].asString(),
                                      BCInfoValue["directory"].asString(),
                                      BCInfoValue["out_command"].asString(),
                                      BCInfoValue["out_file"].asString()};
    }
    auto Loaded = std::make_unique<ProjectEntry>(
        JsonRoot["project_name"].asString(), JsonRoot["project_dir"].asString(),
        BinaryInfo);
    return Loaded;
  } catch (...) {
    return nullptr;
  }
}

const BCInfo *ProjectEntry::getBCInfo(std::string BinaryName) const {
  try {
    return &BinaryInfo.at(BinaryName);
  } catch (const std::out_of_range &) {
    return nullptr;
  }
}
