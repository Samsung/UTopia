#include "ftg/generation/FuzzGenReporter.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/utils/FileUtil.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
namespace ftg {

static inline void updateUTProtobufFile(Json::Value &Root,
                                        std::string ProtobufDir) {
  std::string Path = "FuzzArgsProfile.proto";
  if (!ProtobufDir.empty())
    Path = (fs::path(ProtobufDir) / fs::path(Path)).string();
  Root["ProtobufFile"] = Path;
}

FuzzGenReporter::FuzzGenReporter(std::set<std::string> &PublicAPINames)
    : PublicAPINames(PublicAPINames) {
  for (auto &PublicAPIName : PublicAPINames) {
    auto APIReport = std::make_shared<FuzzGenReporter::APIReportT>();
    APIReport->APIName = PublicAPIName;
    APIReport->APIStatus = std::make_pair("", UNINITIALIZED);
    APIReport->HasUT = false;
    APIReportMap.emplace(PublicAPIName, std::move(APIReport));
  }
}

void FuzzGenReporter::addAPI(std::string APIName, FuzzStatus Status,
                             std::string UTName) {
  auto *APIReport = getAPIReport(APIName);
  assert(APIReport && "Unexpected Program State");
  if (APIReport->APIStatus.second < Status) {
    APIReport->APIStatus = std::make_pair(UTName, Status);
  }
  APIReport->CallStatus.emplace_back(UTName, Status);
  APIReport->HasUT = true;
}

void FuzzGenReporter::addFuzzer(const Fuzzer &F) {
  auto UT = F.getUT();
  auto UTFileName =
      util::getNormalizedPath(fs::path(UT.getFilePath()).filename().string());
  addUT(F.getName(), UTFileName, F.getRelativeUTDir(), F.getStatus());

  for (auto &APICall : UT.getAPICalls()) {
    auto APIName = APICall.Name;
    auto APICallStatus = NOT_FUZZABLE_UNIDENTIFIED;

    auto APIArgs = APICall.Args;
    for (auto &APIArg : APIArgs) {
      for (auto DefID : APIArg.DefIDs) {
        if (F.isFuzzableInput(DefID)) {
          APICallStatus = F.getStatus();
          break;
        }
      }
    }
    addAPI(APIName, APICallStatus, F.getName());
  }
}

void FuzzGenReporter::addNoInputAPI(std::string APIName) {
  auto *APIReport = getAPIReport(APIName);
  assert(APIReport && "Unexpected Program State");
  APIReport->APIStatus = std::make_pair("", NOT_FUZZABLE_NO_INPUT);
}

void FuzzGenReporter::addUT(std::string UTName, std::string UTFileName,
                            std::string UTDir, FuzzStatus Status) {
  auto UTReport = std::make_shared<FuzzGenReporter::UTReportT>();
  UTReport->UTName = UTName;
  UTReport->UTFileName = UTFileName;
  UTReport->UTDir = UTDir;
  UTReport->Status = Status;
  if (!UTReportMap.emplace(UTName, std::move(UTReport)).second) {
    llvm::outs() << "[E] Duplicated UT: " << UTName << "\n";
    assert(false && "Unexpected Program State");
  }
}

const std::shared_ptr<const FuzzGenReporter::APIReportT>
FuzzGenReporter::getAPIReport(std::string APIName) const {
  auto Iter = this->APIReportMap.find(APIName);
  assert(Iter != this->APIReportMap.end() && "APIReport Not Found");
  return Iter->second;
}

const std::shared_ptr<const FuzzGenReporter::UTReportT>
FuzzGenReporter::getUTReport(std::string UTName) const {
  auto Iter = this->UTReportMap.find(UTName);
  assert(Iter != this->UTReportMap.end() && "UTReport Not Found");
  return Iter->second;
}

const std::map<std::string, std::shared_ptr<FuzzGenReporter::APIReportT>> &
FuzzGenReporter::getAPIReportMap() const {
  return APIReportMap;
}

const std::map<std::string, std::shared_ptr<FuzzGenReporter::UTReportT>> &
FuzzGenReporter::getUTReportMap() const {
  return UTReportMap;
}

void FuzzGenReporter::saveReport(std::string OutDir) const {
  // Report Stats
  Json::Value StatsJson = exportStatsJson();

  // Report UT and API Information
  Json::Value UTsJson = exportUTsReportJson();
  Json::Value APIsJson = exportAPIsReportJson();

  // Make FuzzGenReport Json
  Json::Value FuzzGenReportJson;
  FuzzGenReportJson["FuzzInputMutator"] = "ProtoBuf";
  FuzzGenReportJson["UT"] = UTsJson;
  FuzzGenReportJson["API"] = APIsJson;
  for (std::string StatsField : StatsJson.getMemberNames()) {
    FuzzGenReportJson[StatsField] = StatsJson[StatsField];
  }

  // Write FuzzGenReport Json File
  Json::StreamWriterBuilder WBuilder;
  std::string FuzzGenReportJsonStr = writeString(WBuilder, FuzzGenReportJson);
  std::string ReportFilePath = OutDir + PATH_SEPARATOR + REPORT_FILENAME;
  if (util::saveFile(ReportFilePath.c_str(), FuzzGenReportJsonStr.c_str())) {
    llvm::outs() << "Write Report : " << ReportFilePath << "\n";
  } else {
    llvm::outs() << "[ERROR] Writing Report Failed: " << ReportFilePath << "\n";
  }
}

Json::Value FuzzGenReporter::exportUTsReportJson() const {
  Json::Value UTsJson = Json::Value(Json::arrayValue);
  for (auto UTReportMapPair : this->UTReportMap) {
    auto UTReport = UTReportMapPair.second;
    Json::Value UTJson;
    UTJson["Name"] = UTReport->UTName;
    UTJson["Status"] = static_cast<std::string>(UTReport->Status);
    // Information for Fuzzer Build
    UTJson["FuzzTestSrc"] = UTReport->UTFileName;
    UTJson["FuzzEntryPath"] = "fuzz_entry.cc";
    if (!UTReport->UTDir.empty()) {
      UTJson["FuzzTestSrc"] =
          UTReport->UTDir + PATH_SEPARATOR + UTJson["FuzzTestSrc"].asString();
      UTJson["FuzzEntryPath"] =
          UTReport->UTDir + PATH_SEPARATOR + UTJson["FuzzEntryPath"].asString();
    }
    updateUTProtobufFile(UTJson, UTReport->UTDir);
    UTsJson.append(UTJson);
  }
  return UTsJson;
}

Json::Value FuzzGenReporter::exportAPIsReportJson() const {
  Json::Value APIsJson = Json::Value(Json::arrayValue);
  for (auto APIReportPair : this->APIReportMap) {
    auto APIReport = APIReportPair.second;
    Json::Value APIJson;
    APIJson["Name"] = APIReport->APIName;
    APIJson["Status"] = static_cast<std::string>(APIReport->APIStatus.second);
    APIJson["StatusList"] = Json::Value(Json::arrayValue); // for debug
    for (auto Status : APIReport->CallStatus) {
      Json::Value StatusJson;
      StatusJson["Status"] = static_cast<std::string>(Status.second);
      StatusJson["StatusFromUT"] = Status.first;
      APIJson["StatusList"].append(StatusJson);
    }
    APIsJson.append(APIJson);
  }
  return APIsJson;
}

Json::Value FuzzGenReporter::exportStatsJson() const {
  Json::Value RetJson;
  size_t UTCountNotAnalyzed = getUTList(UNINITIALIZED).size();
  size_t UTCountNoInput = getUTList(NOT_FUZZABLE_NO_INPUT).size();
  size_t UTCountUnidentified = getUTList(NOT_FUZZABLE_UNIDENTIFIED).size();
  size_t UTCountFuzzSrcNotGenerated =
      getUTList(FUZZABLE_SRC_NOT_GENERATED).size();
  size_t UTCountFuzzSrcGenerated = getUTList(FUZZABLE_SRC_GENERATED).size();
  size_t UTCountNotFuzzable =
      UTCountNotAnalyzed + UTCountNoInput + UTCountUnidentified;
  size_t UTCountFuzzable = UTCountFuzzSrcNotGenerated + UTCountFuzzSrcGenerated;
  size_t UTCountTotal = UTCountNotFuzzable + UTCountFuzzable;

  RetJson["UTCount_Total"] = UTCountTotal;
  RetJson["UTCount_NoCallSequence"] = UTCountNotAnalyzed;
  RetJson["UTCount_NoInputParam"] = UTCountNoInput;
  RetJson["UTCount_UnidentifiedParam"] = UTCountUnidentified;
  RetJson["UTCount_FuzzSrcNotGenerated"] = UTCountFuzzSrcNotGenerated;
  RetJson["UTCount_FuzzSrcGenerated"] = UTCountFuzzSrcGenerated;
  RetJson["UTCount_NotFuzzable"] = UTCountNotFuzzable;
  RetJson["UTCount_Fuzzable"] = UTCountFuzzable;

  size_t APICountInputParamNotInUT = getAPIList(UNINITIALIZED).size();
  size_t APICountNoInputParam = getAPIList(NOT_FUZZABLE_NO_INPUT).size();
  size_t APICountUnidentifiedParam =
      getAPIList(NOT_FUZZABLE_UNIDENTIFIED).size();
  size_t APICountFuzzSrcNotGenerated =
      getAPIList(FUZZABLE_SRC_NOT_GENERATED).size();
  size_t APICountFuzzSrcGenerated = getAPIList(FUZZABLE_SRC_GENERATED).size();
  size_t APICountNotFuzzable = APICountInputParamNotInUT +
                               APICountNoInputParam + APICountUnidentifiedParam;
  size_t APICountFuzzable =
      APICountFuzzSrcNotGenerated + APICountFuzzSrcGenerated;
  size_t APICountTotal = APICountNotFuzzable + APICountFuzzable;

  size_t APICountUTAnalyzed = 0;
  for (auto APIReport : this->APIReportMap) {
    if (APIReport.second->HasUT) {
      APICountUTAnalyzed++;
    }
  }

  RetJson["APICount_Total"] = APICountTotal;
  RetJson["APICount_NoUT"] = APICountInputParamNotInUT;
  RetJson["APICount_NoInputParam"] = APICountNoInputParam;
  RetJson["APICount_UnidentifiedParam"] = APICountUnidentifiedParam;
  RetJson["APICount_FuzzSrcNotGenerated"] = APICountFuzzSrcNotGenerated;
  RetJson["APICount_FuzzSrcGenerated"] = APICountFuzzSrcGenerated;
  RetJson["APICount_NotFuzzable"] = APICountNotFuzzable;
  RetJson["APICount_Fuzzable"] = APICountFuzzable;
  RetJson["APICount_InCallSequence"] = APICountUTAnalyzed;

  // To be updated by FTG Helper
  RetJson["UTCount_FuzzBuildFail"] = 0;
  RetJson["UTCount_FuzzBuildSuccess"] = 0;
  RetJson["APICount_FuzzBuildSuccess"] = 0;
  RetJson["APICount_FuzzBuildFail"] = 0;

  // printStats
  llvm::outs() << "FuzzGenStats:\n" << RetJson.toStyledString() << '\n';

  // verifyStats
  assert(APICountTotal == this->APIReportMap.size() &&
         "APICountTotal should meet this verification!");
  assert(UTCountTotal == this->UTReportMap.size() &&
         "UTCountTotal should meet this verification!");

  return RetJson;
}

FuzzGenReporter::APIReportT *
FuzzGenReporter::getAPIReport(std::string APIName) {
  auto Acc = APIReportMap.find(APIName);
  if (Acc != APIReportMap.end()) {
    return Acc->second.get();
  }
  return nullptr;
}

std::vector<std::string> FuzzGenReporter::getAPIList(FuzzStatus Status) const {
  std::vector<std::string> Ret;
  for (auto &APIReportPair : this->APIReportMap) {
    auto APIReport = APIReportPair.second;
    if (APIReport->APIStatus.second == Status) {
      Ret.emplace_back(APIReport->APIName);
    }
  }
  return Ret;
}

std::vector<std::string> FuzzGenReporter::getUTList(FuzzStatus Status) const {
  std::vector<std::string> Ret;
  for (auto UTReportMapPair : this->UTReportMap) {
    auto UTReport = UTReportMapPair.second;
    if (UTReport->Status == Status) {
      Ret.emplace_back(UTReport->UTName);
    }
  }
  return Ret;
}

} // namespace ftg
