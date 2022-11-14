#ifndef FTG_TARGETANALYSIS_TARGETLIB_H
#define FTG_TARGETANALYSIS_TARGETLIB_H

#include "ftg/JsonSerializable.h"
#include "ftg/type/GlobalDef.h"
#include "clang/AST/Type.h"
#include "llvm/IR/Module.h"
#include <set>

namespace ftg {

class TargetLib : JsonSerializable {
public:
  TargetLib() = default;
  TargetLib(std::set<std::string> APIs);
  const std::set<std::string> getAPIs() const;
  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;

private:
  std::set<std::string> APIs;
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIB_H
