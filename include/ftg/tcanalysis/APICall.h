#ifndef FTG_TCANALYSIS_APICALL_H
#define FTG_TCANALYSIS_APICALL_H

#include "ftg/utils/json/json.h"
#include "llvm/IR/InstrTypes.h"
#include <set>

namespace ftg {

struct APIArgument {
  std::set<unsigned> DefIDs;

  APIArgument(std::set<unsigned> DefIDs);
  Json::Value toJson() const;
};

struct APICall {
  std::vector<APIArgument> Args;
  size_t Column;
  size_t Line;
  std::string Name;
  std::string Path;

  APICall(llvm::CallBase &CB, std::vector<APIArgument> &Args);
  APICall(const Json::Value &Json);
  Json::Value toJson() const;
};

} // namespace ftg

#endif // FTG_TCANALYSIS_APICALL_H
