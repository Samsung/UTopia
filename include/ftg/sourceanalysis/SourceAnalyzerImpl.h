#ifndef FTG_SOURCEANALYSIS_SOURCEANALYZERIMPL_H
#define FTG_SOURCEANALYSIS_SOURCEANALYZERIMPL_H

#include "ftg/sourceanalysis/SourceAnalysisReport.h"
#include "ftg/sourceanalysis/SourceAnalyzer.h"
#include "ftg/sourceloader/SourceCollection.h"

namespace ftg {

class SourceAnalyzerImpl : public SourceAnalyzer {
public:
  SourceAnalyzerImpl(const SourceCollection &SC);
  std::unique_ptr<AnalyzerReport> getReport() override;
  const SourceAnalysisReport &getActualReport() const;

private:
  SourceAnalysisReport Report;
  unsigned getEndOffset(const clang::ASTUnit &U) const;
  std::vector<std::string> getIncludes(const clang::ASTUnit &U) const;
  const clang::FunctionDecl *getMainFuncDecl(const clang::ASTUnit &U) const;
};

} // namespace ftg

#endif // FTG_SOURCEANALYSIS_SOURCEANALYZERIMPL_H
