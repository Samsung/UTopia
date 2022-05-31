//===-- ConstAnalyzer.h - Analyzer that finds constant in code --*- C++ -*-===//
///
/// \file
/// Defines ConstantAnalyzer which finds constant in code using AST.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_CONSTANTANALYSIS_CONSTANALYZER_H
#define FTG_CONSTANTANALYSIS_CONSTANALYZER_H

#include "ftg/Analyzer.h"
#include "ftg/constantanalysis/ConstAnalyzerReport.h"
#include "clang/Frontend/ASTUnit.h"
#include <memory>

namespace ftg {
class ConstAnalyzer : public Analyzer {
public:
  ConstAnalyzer(const std::vector<clang::ASTUnit *> &ASTUnits)
      : ASTUnits(ASTUnits) {}
  /// \copybrief Analyzer::getReport()
  /// \return std::unique_ptr<ConstantAnalyzerReport>
  std::unique_ptr<AnalyzerReport> getReport() override;

  /// Extract constants defined in given AST
  /// \param[in] AST
  /// \return vector of extracted constant name, value pairs
  static std::vector<std::pair<std::string, ASTValue>>
  extractConst(clang::ASTUnit &AST);

private:
  std::vector<clang::ASTUnit *> ASTUnits;
};
} // namespace ftg
#endif // FTG_CONSTANTANALYSIS_CONSTANALYZER_H
