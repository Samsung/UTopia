#include "RDBasicTypes.h"

namespace ftg {

RDParamIndex::RDParamIndex(std::string FuncName, int ParamNo)
    : FuncName(FuncName), ParamNo(ParamNo) {}

RDParamIndex::RDParamIndex() : FuncName(NONE), ParamNo(0) {}

bool RDParamIndex::isNone() const { return FuncName == NONE; }

bool RDParamIndex::operator<(const RDParamIndex &RHS) const {
  if (FuncName != RHS.FuncName)
    return FuncName < RHS.FuncName;
  return ParamNo < RHS.ParamNo;
}

RDArgIndex::RDArgIndex(llvm::CallBase &CB, unsigned ArgNo)
    : CB(&CB), ArgNo(ArgNo) {}

RDArgIndex::RDArgIndex() : CB(nullptr), ArgNo(0) {}

bool RDArgIndex::isNone() const { return !!CB; }

std::string RDArgIndex::getFunctionName() const {

  if (!CB)
    return "";

  auto *F = CB->getCalledFunction();
  if (!F)
    return "";

  return F->getName();
}

bool RDArgIndex::operator<(const RDArgIndex &RHS) const {

  if (CB != RHS.CB)
    return CB < RHS.CB;
  return ArgNo < RHS.ArgNo;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const RDArgIndex &RHS) {

  if (RHS.CB)
    O << *RHS.CB << "(" << std::to_string(RHS.ArgNo) << ")\n";
  else
    O << "null (" << std::to_string(RHS.ArgNo) << ")\n";
  return O;
}

} // end namespace ftg
