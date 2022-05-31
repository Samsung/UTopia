#include "ftg/utanalysis/UTLoader.h"
#include "ftg/propanalysis/AllocAnalyzer.h"
#include "ftg/sourceloader/BuildDBLoader.h"
#include "ftg/utils/BuildDBParser.h"
#include "ftg/utils/FileUtil.h"

#include "clang/Frontend/CompilerInstance.h"
#include "llvm/IRReader/IRReader.h"

#include <experimental/filesystem>

using namespace ftg;
namespace fs = std::experimental::filesystem;

UTLoader::UTLoader(std::shared_ptr<SourceLoader> SrcLoader,
                   std::shared_ptr<APILoader> APILoader,
                   std::vector<std::string> ReportPaths) {
  if (SrcLoader)
    Source = SrcLoader->load();

  if (APILoader)
    APIs = APILoader->load();

  for (auto &Path : ReportPaths) {
    if (!fs::is_directory(Path))
      continue;

    for (auto &ReportFile : util::readDirectory(Path)) {
      auto JsonValue = util::parseJsonFileToJsonValue(ReportFile.c_str());
      AllocReport.fromJson(JsonValue);
      ArrayReport.fromJson(JsonValue);
      ConstReport.fromJson(JsonValue);
      DirectionReport.fromJson(JsonValue);
      FilePathReport.fromJson(JsonValue);
      LoopReport.fromJson(JsonValue);
    }
  }

  if (Source) {
    AllocAnalyzer AAnalyzer(Source->getLLVMModule(), &AllocReport);
    AllocReport = AAnalyzer.result();
  }
}

const std::set<std::string> &UTLoader::getAPIs() const { return APIs; }

const AllocAnalysisReport &UTLoader::getAllocReport() const {
  return AllocReport;
}

const ArrayAnalysisReport &UTLoader::getArrayReport() const {
  return ArrayReport;
}

const ConstAnalyzerReport &UTLoader::getConstReport() const {
  return ConstReport;
}

const DirectionAnalysisReport &UTLoader::getDirectionReport() const {
  return DirectionReport;
}

const FilePathAnalysisReport &UTLoader::getFilePathReport() const {
  return FilePathReport;
}

const LoopAnalysisReport &UTLoader::getLoopReport() const { return LoopReport; }

const SourceCollection &UTLoader::getSourceCollection() const {
  assert(Source && "Unexpected Program State");
  return *Source;
}

void UTLoader::setAllocReport(const AllocAnalysisReport &Report) {
  AllocReport = Report;
}

void UTLoader::setArrayReport(const ArrayAnalysisReport &Report) {
  ArrayReport = Report;
}

void UTLoader::setConstReport(const ConstAnalyzerReport &Report) {
  ConstReport = Report;
}

void UTLoader::setDirectionReport(const DirectionAnalysisReport &Report) {
  DirectionReport = Report;
}

void UTLoader::setFilePathReport(const FilePathAnalysisReport &Report) {
  FilePathReport = Report;
}

void UTLoader::setLoopReport(const LoopAnalysisReport &Report) {
  LoopReport = Report;
}
