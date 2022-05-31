//===-- AnalyzerReport.h - abstract class of analyzer result ----*- C++ -*-===//
///
/// \file
/// Defines abstract class that contains result of analyzer.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_ANALYZERREPORT_H
#define FTG_ANALYZERREPORT_H

#include "ftg/JsonSerializable.h"

#include <string>

namespace ftg {
class AnalyzerReport : public JsonSerializable {
public:
  virtual ~AnalyzerReport() = default;
  virtual const std::string getReportType() const = 0;
  virtual Json::Value toJson() const override = 0;
  virtual bool fromJson(Json::Value Report) override = 0;
};
} // namespace ftg

#endif // FTG_ANALYZERREPORT_H
