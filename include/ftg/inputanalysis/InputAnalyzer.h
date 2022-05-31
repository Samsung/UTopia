#ifndef FTG_INPUTANALYSIS_INPUTANALYZER_H
#define FTG_INPUTANALYSIS_INPUTANALYZER_H

#include "InputAnalysisReport.h"
#include "ftg/Analyzer.h"

namespace ftg {

class InputAnalyzer : public Analyzer {

public:
  std::unique_ptr<AnalyzerReport> getReport();
  InputAnalysisReport &get() const;

protected:
  std::unique_ptr<InputAnalysisReport> Report;

  virtual void analyze() = 0;
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_INPUTANALYZER_H
