#ifndef FTG_PROPANALYSIS_STRUCTINFO_H
#define FTG_PROPANALYSIS_STRUCTINFO_H

#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/targetanalysis/ParamReport.h"
#include "llvm/IR/DerivedTypes.h"
#include <map>

namespace ftg {

class ArgFlow;

class StructInfo {

public:
  ArgDir FieldsDirection = Dir_NoOp;
  ArgAlloc FieldsAllocation = Alloc_None;
  std::map<unsigned, std::shared_ptr<ArgFlow>> FieldResults;
  llvm::StructType *StructType;
  StructInfo(llvm::StructType *ST);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_STRUCTINFO_H
