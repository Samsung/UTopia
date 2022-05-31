#include "ftg/apiloader/APIExportsNDepsJsonLoader.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/json/json.h"
#include <assert.h>

namespace ftg {

const std::set<std::string> APIExportsNDepsJsonLoader::load() {
  std::set<std::string> APINames;
  Json::Value ExportsNDepsJson =
      util::parseJsonFileToJsonValue(JsonFilePath.c_str());

  assert(ExportsNDepsJson && ExportsNDepsJson.isMember("exported_funcs") &&
         ExportsNDepsJson["exported_funcs"].isMember(LibName) &&
         "Invalid Json");

  for (Json::Value FuncName : ExportsNDepsJson["exported_funcs"][LibName]) {
    APINames.insert(FuncName.asString());
  }
  return APINames;
}

} // namespace ftg
