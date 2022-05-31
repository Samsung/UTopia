#include "ftg/inputanalysis/InputAnalysisReport.h"
#include "ftg/inputanalysis/DefMapGenerator.h"

namespace ftg {

InputAnalysisReport::InputAnalysisReport(
    const std::map<unsigned, std::shared_ptr<Definition>> &DefMap,
    const std::vector<Unittest> &Unittests)
    : DefMap(DefMap), Unittests(Unittests) {}

const std::vector<Unittest> &InputAnalysisReport::getUnittests() const {
  return Unittests;
}

const std::map<unsigned, std::shared_ptr<Definition>> &
InputAnalysisReport::getDefMap() const {
  return DefMap;
}

const std::string InputAnalysisReport::getReportType() const {
  return "inputanalysis";
}

Json::Value InputAnalysisReport::toJson() const {
  Json::Value Result;

  Json::Value UnitTestJson;
  for (auto &Unittest : Unittests)
    UnitTestJson.append(Unittest.getJson());
  Result[getReportType()]["unittests"] = UnitTestJson;
  Result[getReportType()]["Definitions"] = toJson(DefMap);
  return Result;
}

bool InputAnalysisReport::fromJson(Json::Value Root) {
  assert(false && "Not Implemented");
}

bool InputAnalysisReport::fromJson(Json::Value &Root, TargetLib &TargetReport) {
  if (!deserialize(Root, TargetReport)) {
    clear();
    return false;
  }
  return true;
}

void InputAnalysisReport::clear() {
  DefMap.clear();
  Unittests.clear();
}

bool InputAnalysisReport::deserialize(Json::Value &Root,
                                      TargetLib &TargetReport) {
  try {
    const auto &ReportJson = Root[getReportType()];
    for (const auto &DefJson : ReportJson["Definitions"]) {
      Definition D;
      if (!D.fromJson(DefJson, TargetReport))
        return false;
      if (!DefMap.emplace(D.ID, std::make_shared<Definition>(D)).second)
        return false;
    }
    for (const auto &UTJson : ReportJson["unittests"])
      Unittests.emplace_back(UTJson);
  } catch (Json::Exception &E) {
    return false;
  }
  return true;
}

Json::Value InputAnalysisReport::toJson(
    const std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const {
  Json::Value Result = Json::Value(Json::arrayValue);
  for (const auto &Iter : DefMap) {
    const auto &Def = Iter.second;
    assert(Def && "Unexpected Program State");
    Result.append(Def->toJson());
  }
  return Result;
}

} // namespace ftg
