#ifndef FTG_ROOTDEFANALYSIS_RDBASICTYPES_H
#define FTG_ROOTDEFANALYSIS_RDBASICTYPES_H

#include "llvm/IR/InstrTypes.h"
#include <string>

namespace ftg {

struct RDParamIndex {
  const std::string NONE = "#RDParamIndex#None";
  std::string FuncName;
  int ParamNo;

  RDParamIndex(std::string FuncName, int ParamNo);
  RDParamIndex();
  bool isNone() const;
  bool operator<(const RDParamIndex &RHS) const;

  template <typename T> friend T &operator<<(T &O, const RDParamIndex &RHS) {
    O << RHS.FuncName << "(" << RHS.ParamNo << ")";
    return O;
  };
};

struct RDArgIndex {
  llvm::CallBase *CB;
  unsigned ArgNo;

  RDArgIndex(llvm::CallBase &CB, unsigned ArgNo);
  RDArgIndex();

  bool isNone() const;

  std::string getFunctionName() const;

  bool operator<(const RDArgIndex &RHS) const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O,
                                       const RDArgIndex &RHS);
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDBASICTYPES_H
