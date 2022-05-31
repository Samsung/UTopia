//===-- Analyzer.h - abstract class of analyzer -----------------*- C++ -*-===//
///
/// \file
/// Defines abstract class of analyzer.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_ANALYZER_H
#define FTG_ANALYZER_H

#include "ftg/AnalyzerReport.h"

#include <memory>

namespace ftg {
class Analyzer {
public:
  virtual ~Analyzer() = default;
  /// Returns pointer of a report instance that contains its analysis result.
  /// The report will be derived class of AnalyzerReport.
  virtual std::unique_ptr<AnalyzerReport> getReport() = 0;
};
} // namespace ftg

#endif // FTG_ANALYZER_H
