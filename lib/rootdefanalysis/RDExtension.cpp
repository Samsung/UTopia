#include "RDExtension.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace ftg {

void RDExtension::setDirectionReport(
    std::shared_ptr<DirectionAnalysisReport> DirectionReport) {
  this->DirectionReport = DirectionReport;
}

void RDExtension::addTermination(std::string FuncName, size_t ParamNo) {
  Terminations.emplace(FuncName, ParamNo);
}

void RDExtension::addNonStaticClassMethod(std::string FuncName) {

  NonStaticCXXMethod.insert(FuncName);
}

bool RDExtension::isOutParam(const Argument &A) const {
  if (!DirectionReport || !DirectionReport->has(A))
    return false;
  return DirectionReport->get(A) == Dir_Out;
}

bool RDExtension::isTermination(const CallBase &CB, size_t ParamNo) const {

  auto *F = CB.getCalledFunction();
  if (!F)
    return false;

  auto FuncName = F->getName();
  return Terminations.find(RDParamIndex(FuncName, ParamNo)) !=
         Terminations.end();
}

bool RDExtension::isNonStaticClassMethod(std::string FuncName) const {

  return NonStaticCXXMethod.find(FuncName) != NonStaticCXXMethod.end();
}

RDExtension &RDExtension::operator=(const RDExtension &Src) {
  OutParams = Src.OutParams;
  Terminations = Src.Terminations;
  NonStaticCXXMethod = Src.NonStaticCXXMethod;
  DirectionReport = Src.DirectionReport;
  return *this;
}

} // end namespace ftg
