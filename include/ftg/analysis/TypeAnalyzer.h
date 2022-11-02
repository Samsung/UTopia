#ifndef FTG_ANALYSIS_TYPEANALYZER_H
#define FTG_ANALYSIS_TYPEANALYZER_H

#include "ftg/Analyzer.h"
#include "ftg/analysis/TypeAnalysisReport.h"
#include <clang/Frontend/ASTUnit.h>

namespace ftg {

class TypeAnalyzer : public Analyzer {
public:
  TypeAnalyzer(const std::vector<clang::ASTUnit *> &Units);
  std::unique_ptr<AnalyzerReport> getReport() override;

private:
  TypeAnalysisReport Report;
  void collectEnums(const clang::ASTUnit &Unit);
};

} // namespace ftg

#endif // FTG_ANALYSIS_TYPEANALYZER_H
