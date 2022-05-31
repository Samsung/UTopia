#include "ftg/tcanalysis/APICall.h"
#include "ftg/astirmap/IRNode.h"

using namespace ftg;
using namespace llvm;

APIArgument::APIArgument(std::set<unsigned> DefIDs) : DefIDs(DefIDs) {}

Json::Value APIArgument::toJson() const {

  Json::Value Result = Json::Value(Json::arrayValue);
  for (auto DefID : DefIDs)
    Result.append(DefID);

  return Result;
}

APICall::APICall(llvm::CallBase &CB, std::vector<APIArgument> &Args)
    : Args(Args) {
  auto *F =
      dyn_cast_or_null<Function>(CB.getCalledValue()->stripPointerCasts());
  assert(F && "Unexpected Program State");

  Name = std::string(F->getName());

  IRNode Node(CB);
  const auto &Index = Node.getIndex();
  Path = Index.getPath();
  Line = Index.getLine();
  Column = Index.getColumn();
}

APICall::APICall(const Json::Value &Json) {
  assert((Json.isMember("args") && Json["args"].isArray()) &&
         Json.isMember("column") && Json.isMember("line") &&
         Json.isMember("name") && Json.isMember("path") &&
         "Unexpected Program State");

  for (const auto &ArgJson : Json["args"]) {
    assert(ArgJson.isArray() && "Unexpected Program State");

    std::set<unsigned> DefIDs;
    for (auto &DefIDJson : ArgJson)
      DefIDs.insert(DefIDJson.asUInt());
    Args.emplace_back(DefIDs);
  }

  Column = Json["column"].asUInt();
  Line = Json["line"].asUInt();
  Name = Json["name"].asString();
  Path = Json["path"].asString();
}

Json::Value APICall::toJson() const {
  Json::Value Result;
  Result["name"] = Name;
  Result["path"] = Path;
  Result["line"] = Line;
  Result["column"] = Column;

  Json::Value JsonArgs = Json::Value(Json::arrayValue);
  for (auto &Arg : Args)
    JsonArgs.append(Arg.toJson());
  Result["args"] = JsonArgs;

  return Result;
}
