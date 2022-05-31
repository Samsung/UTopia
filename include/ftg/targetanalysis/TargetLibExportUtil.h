#ifndef FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H
#define FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H

#include "ftg/targetanalysis/TargetLib.h"

namespace ftg {

std::string toJsonString(TargetLib &Src);
std::string toJsonString(Type &Src);

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H
