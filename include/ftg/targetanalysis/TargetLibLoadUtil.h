#ifndef FTG_TARGETANALYSIS_TARGETLIBLOADUTIL_H
#define FTG_TARGETANALYSIS_TARGETLIBLOADUTIL_H

#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/Type.h"

namespace ftg {

std::shared_ptr<Type> typeFromJson(const std::string &JsonString,
                                   TargetLib *Report);

class TargetLibLoader {
public:
  TargetLibLoader();
  bool load(const std::string &JsonString);
  std::unique_ptr<TargetLib> takeReport();

private:
  std::unique_ptr<TargetLib> Report;
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIBLOADUTIL_H
