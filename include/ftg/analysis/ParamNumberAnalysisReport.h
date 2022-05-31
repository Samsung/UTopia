#ifndef FTG_ANALYSIS_PARAMNUMBERANALYSISREPORT_H
#define FTG_ANALYSIS_PARAMNUMBERANALYSISREPORT_H

#include "ftg/AnalyzerReport.h"

namespace ftg {

class ParamNumberAnalysisReport : public AnalyzerReport {

public:
  ParamNumberAnalysisReport();
  ParamNumberAnalysisReport(const ParamNumberAnalysisReport &Report);
  const std::string getReportType() const override;
  Json::Value toJson() const override;
  bool fromJson(Json::Value Report) override;
  void add(std::string FuncName, unsigned ParamNum, unsigned BeginIndex);
  const std::map<std::string, unsigned> &getParamBeginMap() const;
  const std::map<std::string, unsigned> &getParamSizeMap() const;

private:
  std::map<std::string, unsigned> ParamBeginMap;
  std::map<std::string, unsigned> ParamSizeMap;
};

} // namespace ftg

#endif // FTG_ANALYSIS_PARAMNUMBERANALYSISREPORT_H
