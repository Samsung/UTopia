#ifndef FTG_UTANALYSIS_UTLOADER_H
#define FTG_UTANALYSIS_UTLOADER_H

#include "ftg/analysis/TypeAnalysisReport.h"
#include "ftg/apiloader/APILoader.h"
#include "ftg/constantanalysis/ConstAnalyzerReport.h"
#include "ftg/propanalysis/AllocSizeAnalysisReport.h"
#include "ftg/propanalysis/ArrayAnalysisReport.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include "ftg/propanalysis/FilePathAnalysisReport.h"
#include "ftg/propanalysis/LoopAnalysisReport.h"
#include "ftg/sourceloader/SourceLoader.h"

namespace ftg {
/**
 * @brief Load and Manage IR and AST list
 * @details
 */
class UTLoader {

public:
  UTLoader(std::shared_ptr<SourceLoader> SrcLoader = nullptr,
           std::shared_ptr<APILoader> APILoader = nullptr,
           std::vector<std::string> ReportPaths = {});

  const std::set<std::string> &getAPIs() const;
  const AllocSizeAnalysisReport &getAllocSizeReport() const;
  const ArrayAnalysisReport &getArrayReport() const;
  const ConstAnalyzerReport &getConstReport() const;
  const DirectionAnalysisReport &getDirectionReport() const;
  const FilePathAnalysisReport &getFilePathReport() const;
  const LoopAnalysisReport &getLoopReport() const;
  const SourceCollection &getSourceCollection() const;
  const TypeAnalysisReport &getTypeReport() const;
  void setAllocSizeReport(const AllocSizeAnalysisReport &Report);
  void setArrayReport(const ArrayAnalysisReport &Report);
  void setConstReport(const ConstAnalyzerReport &Report);
  void setDirectionReport(const DirectionAnalysisReport &Report);
  void setFilePathReport(const FilePathAnalysisReport &Report);
  void setLoopReport(const LoopAnalysisReport &Report);

private:
  std::set<std::string> APIs;
  AllocSizeAnalysisReport AllocSizeReport;
  ArrayAnalysisReport ArrayReport;
  ConstAnalyzerReport ConstReport;
  DirectionAnalysisReport DirectionReport;
  FilePathAnalysisReport FilePathReport;
  LoopAnalysisReport LoopReport;
  TypeAnalysisReport TypeReport;
  std::unique_ptr<SourceCollection> Source;
};
} // namespace ftg

#endif // FTG_UTANALYSIS_UTLOADER_H
