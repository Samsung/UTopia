#ifndef FTG_ANALYSIS_TYPEANALYSISREPORT_H
#define FTG_ANALYSIS_TYPEANALYSISREPORT_H

#include "ftg/AnalyzerReport.h"
#include "ftg/type/GlobalDef.h"

namespace ftg {

class TypeAnalysisReport : public AnalyzerReport {
public:
  TypeAnalysisReport(const TypeAnalysisReport *PreReport = nullptr);
  void addEnum(const clang::EnumDecl &D);
  bool fromJson(Json::Value Report) override;
  const std::map<std::string, std::shared_ptr<Enum>> &getEnums() const;
  const std::string getReportType() const;
  Json::Value toJson() const override;
  TypeAnalysisReport &operator+=(const TypeAnalysisReport &Report);

private:
  std::map<std::string, std::shared_ptr<Enum>> Enums;
};

} // namespace ftg

#endif // FTG_ANALYSIS_TYPEANALYSISREPORT_H
