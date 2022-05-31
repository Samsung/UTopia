#ifndef FTG_PROPANALYSIS_PROPANALYZER_H
#define FTG_PROPANALYSIS_PROPANALYZER_H

#include "ftg/Analyzer.h"
#include "llvm/IR/Argument.h"

namespace ftg {

class PropAnalyzer : public Analyzer {

public:
  virtual ~PropAnalyzer() = default;
  virtual void analyze(const llvm::Argument &A) = 0;
  virtual std::unique_ptr<AnalyzerReport> getReport() = 0;
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_PROPANALYZER_H
