#include "ftg/generation/GenLoader.h"
#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/utils/FileUtil.h"

namespace ftg {

GenLoader::GenLoader() = default;

bool GenLoader::load(std::string TargetReportDir, std::string UTReportPath) {
  if (!loadTarget(TargetReportDir))
    return false;
  return loadUT(UTReportPath);
}

const TargetLib &GenLoader::getTargetReport() const {
  assert(TargetReport && "Unexpected Program State");
  return *TargetReport;
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

bool GenLoader::loadTarget(const std::string &TargetReportDir) {
  TargetLibLoader Loader;
  std::vector<std::string> ReportPath;
  try {
    for (auto TargetReportFile : util::readDirectory(TargetReportDir)) {
      auto JsonString = util::readFile(TargetReportFile.c_str());
      Loader.load(JsonString);

      std::istringstream Iss(JsonString);
      Json::CharReaderBuilder Reader;
      Json::Value JsonValue;
      Json::parseFromStream(Reader, Iss, &JsonValue, nullptr);
      DirectionReport.fromJson(JsonValue);
      ParamNumberReport.fromJson(JsonValue);
    }
  } catch (std::invalid_argument &E) {
    return false;
  }
  TargetReport = Loader.takeReport();
  return true;
}

bool GenLoader::loadUT(const std::string &UTReportPath) {
  if (!TargetReport)
    return false;

  auto JsonValue = util::parseJsonFileToJsonValue(UTReportPath.c_str());
  InputReport.fromJson(JsonValue, *TargetReport);
  SourceReport.fromJson(JsonValue);
  return true;
}

} // namespace ftg
