#include "ftg/apiloader/APIJsonLoader.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include "json/json.h"
#include <assert.h>

namespace ftg {

bool APIJsonLoader::isDeprecatedLibrary(std::string LibName) {
  return !util::regex(LibName, ".+_deprecated").empty();
}

const std::set<std::string> APIJsonLoader::load() {
  std::set<std::string> APINames;
  Json::Value PublicAPIJson =
      util::parseJsonFileToJsonValue(this->JsonFilePath.c_str());

  assert(PublicAPIJson && "Invalid Json");

  for (std::string &LibName : PublicAPIJson.getMemberNames()) {
    if (isDeprecatedLibrary(LibName)) {
      continue;
    }

    for (Json::Value &FuncName : PublicAPIJson[LibName]) {
      APINames.insert(FuncName.asString());
    }
  }
  return APINames;
};

} // namespace ftg
