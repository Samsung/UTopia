#ifndef FTG_GENERATION_GENLOADER_H
#define FTG_GENERATION_GENLOADER_H

#include "ftg/analysis/ParamNumberAnalysisReport.h"
#include "ftg/inputanalysis/InputAnalysisReport.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include "ftg/sourceanalysis/SourceAnalysisReport.h"
#include "ftg/targetanalysis/TargetLib.h"

namespace ftg {

class GenLoader {

public:
  GenLoader();
  bool load(std::string TargetReportDir, std::string UTReportPath);
  const DirectionAnalysisReport &getDirectionReport() const;
  const InputAnalysisReport &getInputReport() const;
  const ParamNumberAnalysisReport &getParamNumberReport() const;
  const SourceAnalysisReport &getSourceReport() const;
  const TargetLib &getTargetReport() const;

private:
  std::unique_ptr<TargetLib> TargetReport;
  DirectionAnalysisReport DirectionReport;
  InputAnalysisReport InputReport;
  ParamNumberAnalysisReport ParamNumberReport;
  SourceAnalysisReport SourceReport;

  bool loadTarget(const std::string &TargetReportDir);
  bool loadUT(const std::string &UTReportPath);
};

} // namespace ftg

#endif // FTG_GENERATION_GENLOADER_H
