#include "ParamNumberAnalysisReport.h"
#include "llvm/Support/raw_ostream.h"

namespace ftg {

ParamNumberAnalysisReport::ParamNumberAnalysisReport() = default;

ParamNumberAnalysisReport::ParamNumberAnalysisReport(
    const ParamNumberAnalysisReport &Report) {
  auto SrcParamBeginMap = Report.getParamBeginMap();
  ParamBeginMap.insert(SrcParamBeginMap.begin(), SrcParamBeginMap.end());
  auto SrcParamSizeMap = Report.getParamSizeMap();
  ParamSizeMap.insert(SrcParamSizeMap.begin(), SrcParamSizeMap.end());
}

const std::string ParamNumberAnalysisReport::getReportType() const {
  return "paramnumber";
}

Json::Value ParamNumberAnalysisReport::toJson() const {
  Json::Value ParamBeginMapValue;
  for (auto Iter : ParamBeginMap)
    ParamBeginMapValue[Iter.first] = Iter.second;

  Json::Value ParamSizeMapValue;
  for (auto Iter : ParamSizeMap)
    ParamSizeMapValue[Iter.first] = Iter.second;

  Json::Value JsonValue;
  JsonValue["ParamBeginMap"] = ParamBeginMapValue;
  JsonValue["ParamSizeMap"] = ParamSizeMapValue;

  Json::Value Root;
  Root[getReportType()] = JsonValue;
  return Root;
}

bool ParamNumberAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;

  auto ParamNumberReport = Report[getReportType()];
  if (!ParamNumberReport.isMember("ParamBeginMap") ||
      !ParamNumberReport.isMember("ParamSizeMap"))
    return false;

  auto ParamBeginMapReport = ParamNumberReport["ParamBeginMap"];
  for (auto MemberName : ParamBeginMapReport.getMemberNames()) {
    ParamBeginMap.emplace(MemberName, ParamBeginMapReport[MemberName].asUInt());
  }

  auto ParamSizeMapReport = ParamNumberReport["ParamSizeMap"];
  for (auto MemberName : ParamSizeMapReport.getMemberNames()) {
    ParamSizeMap.emplace(MemberName, ParamSizeMapReport[MemberName].asUInt());
  }
  return true;
}

void ParamNumberAnalysisReport::add(std::string FuncName, unsigned ParamNum,
                                    unsigned BeginIndex) {
  ParamBeginMap.emplace(FuncName, BeginIndex);
  ParamSizeMap.emplace(FuncName, ParamNum);
}

const std::map<std::string, unsigned> &
ParamNumberAnalysisReport::getParamBeginMap() const {
  return ParamBeginMap;
}

const std::map<std::string, unsigned> &
ParamNumberAnalysisReport::getParamSizeMap() const {
  return ParamSizeMap;
}

} // namespace ftg
