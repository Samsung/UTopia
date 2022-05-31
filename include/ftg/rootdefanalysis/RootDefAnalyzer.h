#ifndef FTG_ROOTDEFANALYSIS_ROOTDEFANALYZER_H
#define FTG_ROOTDEFANALYSIS_ROOTDEFANALYZER_H

#include "RDCache.h"
#include "RDExtension.h"
#include "RDSpace.h"

namespace ftg {

class RootDefAnalyzer {

public:
  virtual ~RootDefAnalyzer() = default;
  virtual void
  setSearchSpace(const std::vector<llvm::Function *> &Functions) = 0;
  virtual std::set<RDNode> getRootDefinitions(const llvm::Use &U) = 0;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_ROOTDEFANALYZER_H
