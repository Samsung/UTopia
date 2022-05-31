#include "InputAnalyzer.h"

namespace ftg {

std::unique_ptr<AnalyzerReport> InputAnalyzer::getReport() {
  return move(Report);
}

InputAnalysisReport &InputAnalyzer::get() const {
  assert(Report && "Unexpected Program State");
  return *Report;
}

} // namespace ftg
