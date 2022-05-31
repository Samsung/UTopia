#ifndef FTG_PROPANALYSIS_FILEPATHANALYSISREPORT_H
#define FTG_PROPANALYSIS_FILEPATHANALYSISREPORT_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/ArgPropAnalysisReport.hpp"

namespace ftg {

class FilePathAnalysisReport : public ArgPropAnalysisReport<bool> {

public:
  FilePathAnalysisReport();
  FilePathAnalysisReport(const FilePathAnalysisReport &Report);
  bool fromJson(Json::Value Report);
  const std::string getReportType() const;
  Json::Value toJson() const;
  void add(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_FILEPATHANALYSISREPORT_H
