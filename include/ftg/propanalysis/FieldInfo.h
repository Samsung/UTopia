#ifndef FTG_PROPANALYSIS_FIELDINFO_H
#define FTG_PROPANALYSIS_FIELDINFO_H

#include "ftg/propanalysis/ArgFlow.h"
#include "llvm/IR/Value.h"
#include <set>

namespace ftg {

class ArgFlow;

class FieldInfo {
public:
  unsigned FieldNum;
  std::set<unsigned> SizeFields;
  std::set<unsigned> ArrayFields;
  std::set<llvm::Value *> Values;
  ArgFlow &Parent; // struct result

  FieldInfo(unsigned FieldNum, ArgFlow &Parent);
  bool hasLenRelatedField();
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_FIELDINFO_H
