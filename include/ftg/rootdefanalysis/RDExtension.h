#ifndef FTG_ROOTDEFANALYSIS_RDEXTENSION_H
#define FTG_ROOTDEFANALYSIS_RDEXTENSION_H

#include "RDBasicTypes.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include "llvm/IR/Function.h"
#include <set>
#include <string>
#include <vector>

namespace ftg {

class RDExtension {
public:
  RDExtension() = default;
  void
  setDirectionReport(std::shared_ptr<DirectionAnalysisReport> DirectionReport);
  void addTermination(std::string FuncName, size_t ParamNo);
  void addNonStaticClassMethod(std::string FuncName);
  bool isOutParam(const llvm::Argument &A) const;
  bool isTermination(const llvm::CallBase &CB, size_t ParamNo) const;
  bool isNonStaticClassMethod(std::string FuncName) const;
  RDExtension &operator=(const RDExtension &Src);

private:
  std::set<RDParamIndex> OutParams;
  std::set<RDParamIndex> Terminations;
  std::set<std::string> NonStaticCXXMethod;
  std::shared_ptr<DirectionAnalysisReport> DirectionReport;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDEXTENSION_H
