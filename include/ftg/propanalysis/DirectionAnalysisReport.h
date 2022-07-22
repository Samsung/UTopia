#ifndef FTG_PROPANALYSIS_DIRECTIONANALYSISREPORT_H
#define FTG_PROPANALYSIS_DIRECTIONANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

class DirectionAnalysisReport : public ArgPropAnalysisReport<ArgDir> {

public:
  DirectionAnalysisReport() = default;
  const std::string getReportType() const override;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;
  void add(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_DIRECTIONANALYSISREPORT_H
