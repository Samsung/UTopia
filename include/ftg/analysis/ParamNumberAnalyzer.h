#ifndef FTG_ANALYSIS_PARAMNUMBERANALYZER_H
#define FTG_ANALYSIS_PARAMNUMBERANALYZER_H

#include "ParamNumberAnalysisReport.h"
#include "ftg/Analyzer.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Module.h"
#include <set>

namespace ftg {

class ParamNumberAnalyzer : public Analyzer {

public:
  ParamNumberAnalyzer(const std::vector<clang::ASTUnit *> &ASTUnits,
                      const llvm::Module &M, std::set<std::string> FuncNames);
  std::unique_ptr<AnalyzerReport> getReport() override;

private:
  ParamNumberAnalysisReport Report;
};

}; // namespace ftg

#endif // FTG_ANALYSIS_PARAMNUMBERANALYZER_H
