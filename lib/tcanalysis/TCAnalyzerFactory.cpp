#include "ftg/tcanalysis/TCAnalyzerFactory.h"

namespace ftg {

std::unique_ptr<TCAnalyzerFactory> createTCAnalyzerFactory(std::string Type) {
  if (Type == "tct")
    return std::make_unique<TCTAnalyzerFactory>();
  if (Type == "gtest")
    return std::make_unique<GoogleTestAnalyzerFactory>();
  if (Type == "boost")
    return std::make_unique<BoostAnalyzerFactory>();
  return nullptr;
}

} // namespace ftg
