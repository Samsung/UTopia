#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/utils/ASTUtil.h"
#include "json/json.h"

namespace ftg {

TargetLib::TargetLib(std::set<std::string> APIs) : APIs(APIs) {}

const std::set<std::string> TargetLib::getAPIs() const { return APIs; }

bool TargetLib::fromJson(Json::Value Json) {
  try {
    if (Json.isNull() || Json.empty())
      throw Json::LogicError("Abnormal Json Value");
    for (auto API : Json["APIs"])
      APIs.emplace(API.asString());
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

Json::Value TargetLib::toJson() const {
  Json::Value Json;

  Json["APIs"] = Json::Value(Json::arrayValue);
  for (auto API : APIs)
    Json["APIs"].append(API);
  return Json;
}

} // namespace ftg
