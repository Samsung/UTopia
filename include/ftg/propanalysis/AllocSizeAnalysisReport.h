#ifndef FTG_PROPANALYSIS_ALLOCSIZEANALYSISREPORT_H
#define FTG_PROPANALYSIS_ALLOCSIZEANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

class AllocSizeAnalysisReport : public ArgPropAnalysisReport<bool> {

public:
  AllocSizeAnalysisReport() = default;
  const std::string getReportType() const override;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;
  void add(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ALLOCSIZEANALYSISREPORT_H
