#ifndef FTG_PROPANALYSIS_LOOPANALYSISREPORT_H
#define FTG_PROPANALYSIS_LOOPANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

struct LoopAnalysisResult {
  bool LoopExit;
  unsigned LoopDepth;
};

class LoopAnalysisReport : public ArgPropAnalysisReport<LoopAnalysisResult> {
public:
  void add(ArgFlow &AF, std::vector<int> Indices = {});
  bool fromJson(Json::Value) override;
  const std::string getReportType() const override;
  Json::Value toJson() const override;
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_LOOPANALYSISREPORT_H
