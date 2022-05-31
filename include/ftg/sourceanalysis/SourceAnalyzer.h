#ifndef FTG_SOURCEANALYSIS_SOURCEANALYZER_H
#define FTG_SOURCEANALYSIS_SOURCEANALYZER_H

#include "ftg/Analyzer.h"

namespace ftg {

class SourceAnalyzer : public Analyzer {
public:
  virtual ~SourceAnalyzer() = default;
  virtual std::unique_ptr<AnalyzerReport> getReport() = 0;
};

} // namespace ftg

#endif // FTG_SOURCEANALYSIS_SOURCEANALYZER_H
