#ifndef FTG_SOURCEANALYSIS_SOURCEANALYSISREPORT_H
#define FTG_SOURCEANALYSIS_SOURCEANALYSISREPORT_H

#include "ftg/AnalyzerReport.h"
#include "ftg/utanalysis/Location.h"
#include "clang/Frontend/ASTUnit.h"

namespace ftg {

class SourceAnalysisReport : public AnalyzerReport {

public:
  void addSourceInfo(std::string SourceName, unsigned EndOffset,
                     const std::vector<std::string> &IncludedHeaders);
  bool fromJson(Json::Value Report) override;
  unsigned getEndOffset(std::string SourceName) const;
  const Location &getMainFuncLoc() const;
  std::vector<std::string> getIncludedHeaders(std::string SourceName) const;
  const std::string getReportType() const override;
  std::string getSrcBaseDir() const;
  void setMainFuncLoc(const Location &Loc);
  void setSrcBaseDir(const std::string &SrcBaseDir);
  Json::Value toJson() const override;

private:
  struct SourceInfo {
    unsigned EndOffset;
    std::vector<std::string> IncludedHeaders;
  };

  Location MainFuncLoc;
  std::map<std::string, SourceInfo> SourceInfoMap;
  std::string SrcBaseDir;
};

} // namespace ftg

#endif // FTG_SOURCEANALYSIS_SOURCEANALYSISREPORT_H
