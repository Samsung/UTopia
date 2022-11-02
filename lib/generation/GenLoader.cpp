#include "ftg/generation/GenLoader.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"

namespace ftg {

GenLoader::GenLoader() = default;

bool GenLoader::load(std::string TargetReportDir, std::string UTReportPath) {
  if (!loadTarget(TargetReportDir))
    return false;
  return loadUT(UTReportPath);
}

const DirectionAnalysisReport &GenLoader::getDirectionReport() const {
  return DirectionReport;
}

const InputAnalysisReport &GenLoader::getInputReport() const {
  return InputReport;
}

const ParamNumberAnalysisReport &GenLoader::getParamNumberReport() const {
  return ParamNumberReport;
}

const SourceAnalysisReport &GenLoader::getSourceReport() const {
  return SourceReport;
}

const TypeAnalysisReport &GenLoader::getTypeReport() const {
  return TypeReport;
}

bool GenLoader::loadTarget(const std::string &TargetReportDir) {
  std::vector<std::string> ReportPath;
  try {
    for (auto TargetReportFile : util::readDirectory(TargetReportDir)) {
      auto JsonString = util::readFile(TargetReportFile.c_str());
      Json::Value JsonValue = util::strToJson(JsonString);
      DirectionReport.fromJson(JsonValue);
      ParamNumberReport.fromJson(JsonValue);
      TypeReport.fromJson(JsonValue);
    }
  } catch (std::invalid_argument &E) {
    return false;
  }
  return true;
}

bool GenLoader::loadUT(const std::string &UTReportPath) {
  auto JsonValue = util::parseJsonFileToJsonValue(UTReportPath.c_str());
  // NOTE: TypeReport should be loaded before InputReport.
  TypeReport.fromJson(JsonValue);
  InputReport.fromJson(JsonValue, TypeReport);
  SourceReport.fromJson(JsonValue);
  return true;
}

} // namespace ftg
