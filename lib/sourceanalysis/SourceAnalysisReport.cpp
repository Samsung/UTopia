#include "ftg/sourceanalysis/SourceAnalysisReport.h"

using namespace ftg;

void SourceAnalysisReport::addSourceInfo(
    std::string SourceName, unsigned EndOffset,
    const std::vector<std::string> &IncludedHeaders) {
  SourceInfoMap.emplace(SourceName, SourceInfo{EndOffset, IncludedHeaders});
}

bool SourceAnalysisReport::fromJson(Json::Value Report) {
  if (!Report.isMember(getReportType()))
    return false;

  const auto &SourceAnalysisJson = Report[getReportType()];
  if (SourceAnalysisJson.isMember("mainfuncloc")) {
    const auto &MainFuncLocJson = SourceAnalysisJson["mainfuncloc"];
    MainFuncLoc.setFilePath(MainFuncLocJson["FilePath"].asString());
    MainFuncLoc.setLength(MainFuncLocJson["Length"].asUInt());
    MainFuncLoc.setOffset(MainFuncLocJson["Offset"].asUInt());
  }

  if (SourceAnalysisJson.isMember("sourceinfo")) {
    const auto &SourceInfoArrayJson = SourceAnalysisJson["sourceinfo"];
    for (auto MemberName : SourceInfoArrayJson.getMemberNames()) {
      const auto &SourceInfoJson = SourceInfoArrayJson[MemberName];
      if (!SourceInfoJson.isMember("endoffset") ||
          !SourceInfoJson.isMember("includedheaders"))
        continue;

      std::vector<std::string> IncludedHeaders;
      for (auto &Element : SourceInfoJson["includedheaders"])
        IncludedHeaders.push_back(Element.asString());
      SourceInfoMap.emplace(
          MemberName,
          SourceInfo{SourceInfoJson["endoffset"].asUInt(), IncludedHeaders});
    }
  }

  if (SourceAnalysisJson.isMember("srcbasedir"))
    SrcBaseDir = SourceAnalysisJson["srcbasedir"].asString();

  return true;
}

unsigned SourceAnalysisReport::getEndOffset(std::string SourceName) const {
  auto Iter = SourceInfoMap.find(SourceName);
  if (Iter == SourceInfoMap.end())
    return 0;

  return Iter->second.EndOffset;
}

const Location &SourceAnalysisReport::getMainFuncLoc() const {
  return MainFuncLoc;
}

std::vector<std::string>
SourceAnalysisReport::getIncludedHeaders(std::string SourceName) const {
  auto Iter = SourceInfoMap.find(SourceName);
  if (Iter == SourceInfoMap.end())
    return {};

  return Iter->second.IncludedHeaders;
}

const std::string SourceAnalysisReport::getReportType() const {
  return "sourceanalysis";
}

std::string SourceAnalysisReport::getSrcBaseDir() const { return SrcBaseDir; }

void SourceAnalysisReport::setMainFuncLoc(const Location &Loc) {
  MainFuncLoc = Loc;
}

void SourceAnalysisReport::setSrcBaseDir(const std::string &SrcBaseDir) {
  this->SrcBaseDir = SrcBaseDir;
}

Json::Value SourceAnalysisReport::toJson() const {
  Json::Value Result;
  Result[getReportType()]["mainfuncloc"] = MainFuncLoc.getJsonValue();

  Json::Value SourceInfoJson;
  for (const auto &Iter1 : SourceInfoMap) {
    Json::Value SourceInfoElementJson;
    SourceInfoElementJson["endoffset"] = Iter1.second.EndOffset;
    Json::Value IncludedHeaderElementJson(Json::arrayValue);
    for (const auto &Iter2 : Iter1.second.IncludedHeaders)
      IncludedHeaderElementJson.append(Iter2);
    SourceInfoElementJson["includedheaders"] = IncludedHeaderElementJson;
    Result[getReportType()]["sourceinfo"][Iter1.first] = SourceInfoElementJson;
  }

  Result[getReportType()]["srcbasedir"] = SrcBaseDir;
  return Result;
}
