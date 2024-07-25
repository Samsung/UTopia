#include "ftg/type/GlobalDef.h"
#include <utility>

namespace ftg {

Enum::Enum(std::string Name, std::vector<EnumConst> Enumerators)
    : Name{std::move(Name)}, Enumerators{std::move(Enumerators)} {}

Enum::Enum(const clang::EnumDecl &ClangDecl) {
  auto EnumName = ClangDecl.getQualifiedNameAsString();
  if (auto *TypedefDecl = ClangDecl.getTypedefNameForAnonDecl())
    EnumName = TypedefDecl->getQualifiedNameAsString();
  Name = EnumName;
  for (auto *Const : ClangDecl.enumerators()) {
    auto InitValue = Const->getInitVal();
#if LLVM_VERSION_MAJOR >= 17
    if (!InitValue.isRepresentableByInt64())
      continue;
#endif
    Enumerators.emplace_back(
        EnumConst(Const->getNameAsString(), InitValue.getExtValue()));
  }
}

Enum::Enum(Json::Value Json) {
  assert(fromJson(Json) && "Unexpected Program State");
}

Json::Value Enum::toJson() const {
  Json::Value Json;

  Json["Name"] = Name;
  Json::Value EnumeratorsJson = Json::Value(Json::arrayValue);
  for (auto Enumerator : Enumerators)
    EnumeratorsJson.append(Enumerator.toJson());
  Json["Enumerators"] = EnumeratorsJson;

  return Json;
}

bool Enum::fromJson(Json::Value Json) {
  try {
    if (Json.isNull() || Json.empty() || !Json["Name"].isString() ||
        !Json["Enumerators"].isArray())
      throw Json::LogicError("Abnormal Json Value");
    Name = Json["Name"].asString();
    for (auto EnumeratorJson : Json["Enumerators"])
      Enumerators.emplace_back(EnumeratorJson);
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

std::string Enum::getName() const { return Name; }

const std::vector<EnumConst> &Enum::getElements() const { return Enumerators; }

EnumConst::EnumConst(std::string Name, int64_t Value)
    : Name(Name), Value(Value) {}

EnumConst::EnumConst(Json::Value Json) {
  assert(fromJson(Json) && "Unexpected Program State");
}

std::string EnumConst::getName() const { return Name; }

int64_t EnumConst::getValue() const { return Value; }

Json::Value EnumConst::toJson() const {
  Json::Value Json;

  Json["Name"] = Name;
  Json["Value"] = Value;

  return Json;
}

bool EnumConst::fromJson(Json::Value Json) {
  try {
    if (Json.isNull() || Json.empty() || !Json["Name"].isString() ||
        !Json["Value"].isInt64())
      throw Json::LogicError("Abnormal Json Value");
    Name = Json["Name"].asString();
    Value = Json["Value"].asInt64();
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

} // namespace ftg
