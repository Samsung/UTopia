//===-- ConstAnalyzerReport.h - Result of constant analyzer -----*- C++ -*-===//
///
/// \file
/// Defines class for containing result of constant analyzer.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_CONSTANTANALYSIS_CONSTANALYZERREPORT_H
#define FTG_CONSTANTANALYSIS_CONSTANALYZERREPORT_H

#include "ftg/AnalyzerReport.h"
#include "ftg/constantanalysis/ASTValue.h"
#include <map>

namespace ftg {
class ConstAnalyzerReport : public AnalyzerReport {
public:
  ConstAnalyzerReport() = default;
  ConstAnalyzerReport(std::map<std::string, ASTValue> ConstantMap)
      : ConstantMap(ConstantMap) {}
  const ASTValue *getConstValue(std::string ID) const;
  const std::string getReportType() const override;

  /// Add constant to the report. If constants with same names exist,
  /// only first one will be added. Others will be ignored.
  /// \param[in] Name
  /// \param[in] Value
  /// \return true if added, else false.
  bool addConst(std::string Name, ASTValue Value);
  Json::Value toJson() const override;
  bool fromJson(Json::Value Report) override;

private:
  std::map<std::string, ASTValue> ConstantMap;
};
} // namespace ftg
#endif // FTG_CONSTANTANALYSIS_CONSTANALYZERREPORT_H
