#include "ftg/inputanalysis/Definition.h"
#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/utils/FileUtil.h"

using namespace ftg;

Definition::Definition()
    : ID(0), Offset(0), Length(0), FilePath(false), BufferAllocSize(false),
      Array(false), ArrayLen(false), LoopExit(false),
      Declaration(DeclType_None), AssignOperatorRequired(false), TypeOffset(0),
      EndOffset(0) {}

bool Definition::fromJson(const Json::Value &Root, TargetLib &TargetReport) {
  try {
    Array = Root["Array"].asBool();
    for (auto &ArrayID : Root["ArrayIDs"])
      ArrayIDs.emplace(ArrayID.asUInt());
    ArrayLen = Root["ArrayLen"].asBool();
    for (auto &ArrayLenID : Root["ArrayLenIDs"])
      ArrayLenIDs.emplace(ArrayLenID.asUInt());
    AssignOperatorRequired = Root["AssignOperatorRequired"].asBool();
    BufferAllocSize = Root["BufferAllocSize"].asBool();
    DataType = typeFromJson(Root["DataType"].toStyledString(), &TargetReport);
    Declaration = (Definition::DeclType)Root["Declaration"].asUInt();
    EndOffset = Root["EndOffset"].asUInt();
    FilePath = Root["FilePath"].asBool();
    for (auto &FilterJson : Root["Filters"])
      Filters.emplace(FilterJson.asString());
    ID = Root["ID"].asUInt();
    Namespace = Root["Namespace"].asString();
    Path = util::getNormalizedPath(Root["Path"].asString());
    Offset = Root["Offset"].asUInt();
    Length = Root["Length"].asUInt();
    LoopExit = Root["LoopExit"].asBool();
    TypeOffset = Root["TypeOffset"].asUInt();
    TypeString = Root["TypeString"].asString();
    Value = ASTValue(Root["Value"].toStyledString());
    VarName = Root["VarName"].asString();
  } catch (Json::Exception &E) {
    return false;
  }
  return true;
}

Json::Value Definition::toJson() const {
  Json::Value Result;

  Result["ID"] = ID;
  Result["Path"] = Path;
  Result["Offset"] = Offset;
  Result["Length"] = Length;
  Result["FilePath"] = FilePath;
  Result["BufferAllocSize"] = BufferAllocSize;
  Result["Array"] = Array;
  Result["ArrayLen"] = ArrayLen;

  Json::Value JsonArrayIDs = Json::Value(Json::arrayValue);
  for (auto ArrayID : ArrayIDs)
    JsonArrayIDs.append(ArrayID);
  Result["ArrayIDs"] = JsonArrayIDs;

  Json::Value JsonArrayLenIDs = Json::Value(Json::arrayValue);
  for (auto ArrayLenID : ArrayLenIDs)
    JsonArrayLenIDs.append(ArrayLenID);
  Result["ArrayLenIDs"] = JsonArrayLenIDs;

  Result["LoopExit"] = LoopExit;

  Json::Value TypeJson;
  assert(DataType && "Unexpected Program State");
  auto Serialized = toJsonString(*DataType);
  std::istringstream StrStream(Serialized);
  StrStream >> TypeJson;
  Result["DataType"] = TypeJson;
  Result["Declaration"] = Declaration;
  Result["AssignOperatorRequired"] = AssignOperatorRequired;
  Result["TypeOffset"] = TypeOffset;
  Result["TypeString"] = TypeString;
  Result["EndOffset"] = EndOffset;
  Result["VarName"] = VarName;
  Result["Namespace"] = Namespace;
  Result["Value"] = Value.toJson();

  Json::Value FilterElementJson;
  for (auto Filter : Filters)
    FilterElementJson.append(Filter);
  Result["Filters"] = FilterElementJson;

  return Result;
}
