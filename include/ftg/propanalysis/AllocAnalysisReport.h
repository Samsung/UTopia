#ifndef PROPANALYSIS_ALLOCANALYSISREPORT_H
#define PROPANALYSIS_ALLOCANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

class AllocAnalysisReport : public ArgPropAnalysisReport<ArgAlloc> {

public:
  AllocAnalysisReport() = default;
  const std::string getReportType() const override;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;
  void add(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // PROPANALYSIS_ALLOCANALYSISREPORT_H
