#ifndef FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H
#define FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H

#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/Type.h"
#include "ftg/utils/json/json.h"

namespace ftg {

std::string toJsonString(TargetLib &Src);
std::string toJsonString(Type &Src);
Json::Value typeToJson(Type *T);

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIBEXPORTUTIL_H
