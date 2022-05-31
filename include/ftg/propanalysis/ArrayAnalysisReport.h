#ifndef FTG_PROPANALYSIS_ARRAYANALYSISREPORT_H
#define FTG_PROPANALYSIS_ARRAYANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

class ArrayAnalysisReport : public ArgPropAnalysisReport<int> {

public:
  static const int ARRAY_NOLEN = -1;
  static const int NO_ARRAY = -2;

  ArrayAnalysisReport() = default;
  const std::string getReportType() const override;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;
  void add(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ARRAYANALYSISREPORT_H
