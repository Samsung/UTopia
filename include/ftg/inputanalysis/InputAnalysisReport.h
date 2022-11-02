#ifndef FTG_INPUTANALYSIS_INPUTANALYSISREPORT_H
#define FTG_INPUTANALYSIS_INPUTANALYSISREPORT_H

#include "ftg/AnalyzerReport.h"
#include "ftg/inputanalysis/Definition.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/tcanalysis/Unittest.h"

namespace ftg {

class InputAnalysisReport : public AnalyzerReport {

public:
  InputAnalysisReport() = default;
  InputAnalysisReport(
      const std::map<unsigned, std::shared_ptr<Definition>> &DefMap,
      const std::vector<Unittest> &UnitTests);
  InputAnalysisReport(const InputAnalysisReport &Rhs) = delete;
  const std::map<unsigned, std::shared_ptr<Definition>> &getDefMap() const;
  const std::string getReportType() const override;
  const std::vector<Unittest> &getUnittests() const;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;
  // TODO: Currently, DefManager requires TargetLib when load from Json.
  // We should decouple DefManager and TargetLib, and then use fromJson
  // that is interface function of AnalyzerReport.
  bool fromJson(Json::Value &Root, const TypeAnalysisReport &Report);

private:
  std::map<unsigned, std::shared_ptr<Definition>> DefMap;
  std::vector<Unittest> Unittests;

  void clear();
  bool deserialize(Json::Value &Root, const TypeAnalysisReport &Report);
  Json::Value
  toJson(const std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const;
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_INPUTANALYSISREPORT_H
