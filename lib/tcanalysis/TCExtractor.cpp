#include "ftg/tcanalysis/TCExtractor.h"

#include "llvm/IR/Module.h"

using namespace ftg;

void TCExtractor::load() {
  for (const auto &TC : extractTCs())
    TCs.emplace(TC.getName(), TC);
}

const Unittest *TCExtractor::getTC(std::string TCName) const {
  if (TCs.find(TCName) != TCs.end())
    return &TCs.at(TCName);
  return nullptr;
}

const std::vector<std::string> TCExtractor::getTCNames() const {
  std::vector<std::string> TCNames;
  for (auto const &TC : TCs) {
    TCNames.emplace_back(TC.first);
  }
  return TCNames;
}
