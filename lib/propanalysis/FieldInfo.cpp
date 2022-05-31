#include "ftg/propanalysis/FieldInfo.h"

namespace ftg {

FieldInfo::FieldInfo(unsigned FieldNum, ArgFlow &Parent)
    : FieldNum(FieldNum), Parent(Parent) {}

bool FieldInfo::hasLenRelatedField() {
  return (!SizeFields.empty() || !ArrayFields.empty());
}

} // namespace ftg
