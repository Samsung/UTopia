#include "ftg/analysis/TypeAnalysisReport.h"

using namespace ftg;
using namespace clang;

TypeAnalysisReport::TypeAnalysisReport(const TypeAnalysisReport *PreReport) {
  if (PreReport)
    *this += *PreReport;
}

void TypeAnalysisReport::addEnum(const EnumDecl &D) {
  auto E = std::make_shared<Enum>(D);
  if (!E)
    return;

  Enums.emplace(E->getName(), E);
}

bool TypeAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;

  const auto &Root = Report[getReportType()];
  if (Root.isMember("enums")) {
    const auto &EnumsValue = Root["enums"];
    if (!EnumsValue.isObject())
      return false;

    try {
      for (const auto &Name : EnumsValue.getMemberNames()) {
        Enums.emplace(Name, std::make_shared<Enum>(EnumsValue[Name]));
      }
    } catch (Json::LogicError &E) {
      return false;
    }
  }
  return true;
}

const std::map<std::string, std::shared_ptr<Enum>> &
TypeAnalysisReport::getEnums() const {
  return Enums;
}

const std::string TypeAnalysisReport::getReportType() const {
  return "typeanalysis";
}

Json::Value TypeAnalysisReport::toJson() const {
  Json::Value EnumsValue;
  for (const auto &[EnumName, EnumPtr] : Enums) {
    if (!EnumPtr)
      continue;
    EnumsValue[EnumName] = EnumPtr->toJson();
  }
  Json::Value Root;
  Root[getReportType()]["enums"] = EnumsValue;
  return Root;
}

TypeAnalysisReport &
TypeAnalysisReport::operator+=(const TypeAnalysisReport &Report) {
  Enums.insert(Report.Enums.begin(), Report.Enums.end());
  return *this;
}