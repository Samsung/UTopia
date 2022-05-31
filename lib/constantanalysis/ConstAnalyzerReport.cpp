//===-- ConstAnalyzerReport.cpp - Implementation of ConstAnalyzerReport ---===//

#include "ftg/constantanalysis/ConstAnalyzerReport.h"

using namespace ftg;

const std::string ConstAnalyzerReport::getReportType() const {
  return "Constant";
}

const ASTValue *ConstAnalyzerReport::getConstValue(std::string ID) const {
  return (ConstantMap.find(ID) != ConstantMap.end()) ? &ConstantMap.at(ID)
                                                     : nullptr;
}

bool ConstAnalyzerReport::addConst(std::string Name, ASTValue Value) {
  return ConstantMap.emplace(Name, Value).second;
}

Json::Value ConstAnalyzerReport::toJson() const {
  Json::Value Report;
  for (auto ConstValPair : ConstantMap)
    Report[ConstValPair.first] = ConstValPair.second.toJson();
  Json::Value Wrapper;
  Wrapper[getReportType()] = Report;
  return Wrapper;
}

bool ConstAnalyzerReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;
  auto ConstReport = Report[getReportType()];
  for (auto ConstName : ConstReport.getMemberNames()) {
    ASTValue ASTVal(ConstReport[ConstName]);
    if (ASTVal.isValid())
      addConst(ConstName, ASTVal);
  }
  return !ConstantMap.empty();
}
