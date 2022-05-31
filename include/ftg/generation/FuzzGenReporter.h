#ifndef FTG_GENERATION_FUZZGENREPORTER_H
#define FTG_GENERATION_FUZZGENREPORTER_H

#include "ftg/generation/FuzzStatus.h"
#include "ftg/generation/Fuzzer.h"
#include <map>
#include <set>
#include <vector>

namespace ftg {

class TargetLib;

// This class provides Statistics and Fuzzer Build Information per API and UT.
// Warning: Among them, APICount Stats are also referenced in Dashboard.
class FuzzGenReporter {
public:
  // first  : UTName which has this APICall
  // second : FuzzGenStatus of this APICall
  using APIGenStatus = std::pair<std::string, FuzzStatus>;

  struct APIReportT {
    std::string APIName;
    APIGenStatus APIStatus;
    std::vector<APIGenStatus> CallStatus;
    bool HasUT = false;
  };

  struct UTReportT {
    std::string UTName;
    std::string UTFileName;
    std::string UTDir; // RelativeDir
    FuzzStatus Status;
  };

  FuzzGenReporter(std::set<std::string> &PublicAPINames);
  FuzzGenReporter(const FuzzGenReporter &) = delete;
  void addAPI(std::string APIName, FuzzStatus Status, std::string UTName);
  void addFuzzer(const Fuzzer &F, const TargetLib &TargetReport);
  void addNoInputAPI(std::string APIName);
  void addUT(std::string UTName, std::string UTFileName, std::string UTDir,
             FuzzStatus Status);
  const std::shared_ptr<const APIReportT> getAPIReport(std::string) const;
  const std::shared_ptr<const UTReportT> getUTReport(std::string) const;
  const std::map<std::string, std::shared_ptr<APIReportT>> &
  getAPIReportMap() const;
  const std::map<std::string, std::shared_ptr<UTReportT>> &
  getUTReportMap() const;
  void saveReport(std::string OutDir) const;

private:
  std::set<std::string> PublicAPINames;
  std::map<std::string, std::shared_ptr<FuzzGenReporter::APIReportT>>
      APIReportMap; // key: APIName
  std::map<std::string, std::shared_ptr<FuzzGenReporter::UTReportT>>
      UTReportMap; // key: UTName

  Json::Value exportStatsJson() const;
  Json::Value exportUTsReportJson() const;
  Json::Value exportAPIsReportJson() const;
  FuzzGenReporter::APIReportT *getAPIReport(std::string APIName);
  std::vector<std::string> getAPIList(FuzzStatus) const;
  std::vector<std::string> getUTList(FuzzStatus) const;
};

} // namespace ftg

#endif // FTG_GENERATION_FUZZGENREPORTER_H
