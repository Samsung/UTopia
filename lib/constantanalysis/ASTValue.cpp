#include "ftg/constantanalysis/ASTValue.h"
#include "ftg/utils/StringUtil.h"
#include "clang/AST/ASTContext.h"

using namespace clang;

namespace ftg {

ASTValueData::ASTValueData(VType Type, std::string Value)
    : Type(Type), Value(Value) {}

std::string ASTValueData::getAsString(VType Type) const {

  switch (Type) {
  case VType_INT:
    return "INT";
  case VType_FLOAT:
    return "FLOAT";
  case VType_STRING:
    return "STRING";
  case VType_ARRAY:
    return "ARRAY";
  default:
    assert(false && "Unexpected Program State");
  }
}

bool ASTValueData::operator==(const ASTValueData &Rhs) const {

  return Type == Rhs.Type && Value == Rhs.Value;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const ASTValueData &Rhs) {

  O << "Type: " << Rhs.getAsString(Rhs.Type) << ", Value: " << Rhs.Value
    << "\n";

  return O;
}

ASTValue::ASTValue() : IsArray(false) {}

ASTValue::ASTValue(const clang::Expr &E, const clang::ASTContext &Ctx)
    : IsArray(false) {

  std::tie(IsArray, Values) = getData(E, Ctx);
}

ASTValue::ASTValue(Json::Value Json) : IsArray(false) { fromJson(Json); }

ASTValue::ASTValue(const ASTValue &Rhs) : IsArray(false) { *this = Rhs; }

std::pair<bool, std::vector<ASTValueData>>
ASTValue::getData(const Expr &E, const ASTContext &Ctx) const {

  std::pair<bool, std::vector<ASTValueData>> Result =
      std::make_pair(false, std::vector<ASTValueData>());

  auto QTy = E.getType();
  if (QTy.isNull()) {
    return Result;
  }

  if (QTy->isIntegerType() || QTy->isEnumeralType()) {
    auto Value = getValueAsReal(E, Ctx);
    if (!Value.empty()) {
      Result.second.emplace_back(ASTValueData::VType_INT, Value);
    }
    return Result;
  }

  if (QTy->isRealFloatingType()) {
    auto Value = getValueAsReal(E, Ctx);
    if (!Value.empty()) {
      Result.second.emplace_back(ASTValueData::VType_FLOAT, Value);
    }
    return Result;
  }

  if (QTy->isAnyPointerType()) {
    auto PointeeQTy = QTy->getPointeeType();
    if (!PointeeQTy.getTypePtrOrNull() || !PointeeQTy->isAnyCharacterType()) {
      return Result;
    }

    auto Value = getValueAsString(E, Ctx);
    if (!Value.empty()) {
      Result.second.emplace_back(ASTValueData::VType_STRING, Value);
    }
    return Result;
  }

  if (QTy->isArrayType()) {
    Result.first = true;
    Result.second = getDataForArray(E, Ctx);
  }

  return Result;
}

bool ASTValue::fromJson(Json::Value Json) {

  try {
    IsArray = Json["array"].asBool();
    for (auto &Value : Json["values"]) {
      Values.emplace_back((ASTValueData::VType)Value["type"].asUInt(),
                          Value["value"].asString());
    }
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    IsArray = false;
    Values.clear();
    return false;
  }

  return true;
}

Json::Value ASTValue::toJson() const {

  Json::Value Json;

  Json["array"] = IsArray;

  Json::Value ValuesJson = Json::Value(Json::arrayValue);
  for (auto &Value : Values) {
    Json::Value ValueJson;
    ValueJson["type"] = Value.Type;
    ValueJson["value"] = Value.Value;

    ValuesJson.append(ValueJson);
  }

  Json["values"] = ValuesJson;
  return Json;
}

bool ASTValue::isValid() const { return Values.size() > 0; }

bool ASTValue::operator==(const ASTValue &Rhs) const {

  return (Values == Rhs.getValues()) && (IsArray == Rhs.isArray());
}

ASTValue &ASTValue::operator=(const ASTValue &Rhs) {

  this->IsArray = Rhs.isArray();
  this->Values = Rhs.getValues();
  return *this;
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const ASTValue &Rhs) {

  O << "IsArray: " << Rhs.isArray() << ", ";
  if (Rhs.isArray()) {
    for (int S = 0; S < (int)Rhs.getValues().size(); ++S) {
      O << "Value[" << S << "]: " << Rhs.getValues()[S];
      if (S != (int)Rhs.getValues().size() - 1) {
        O << ", ";
      }
    }
  } else if (Rhs.getValues().size() > 0) {
    O << "Value: " << Rhs.getValues()[0] << "\n";
  }

  return O;
}

std::string ASTValue::getValueAsReal(const Expr &E,
                                     const ASTContext &Ctx) const {

  Expr::EvalResult Result;
  if (!E.isValueDependent() && !E.EvaluateAsRValue(Result, Ctx)) {

    auto *RE = E.IgnoreCasts();
    if (!RE || !isa<DeclRefExpr>(RE))
      return "";

    auto &DRE = *dyn_cast<DeclRefExpr>(RE);
    RE = getValueExpr(DRE, Ctx);
    if (!RE || !RE->EvaluateAsRValue(Result, Ctx)) {
      return "";
    }
  }

  auto T = E.getType();
  assert(T.getTypePtrOrNull() && "Unexpected Program State");

  auto &Val = Result.Val;
  auto Value = Val.getAsString(Ctx, T);

  if (T->isFloatingType()) {
    try {
      Value = std::to_string(std::stod(Value));
    }
    // TODO: Try to handle this exception
    catch (std::exception &E) {
      return "";
    }
  } else if (T->isBooleanType()) {
    if (Value == "true")
      Value = "1";
    if (Value == "false")
      Value = "0";
  }

  return Value;
}

std::string ASTValue::getValueAsString(const Expr &E,
                                       const ASTContext &Ctx) const {

  if (auto *SL = llvm::dyn_cast<StringLiteral>(&E)) {
    return getValue(*SL);
  }

  if (auto *ICE = llvm::dyn_cast<CastExpr>(&E)) {
    return getValueAsString(*ICE->getSubExpr(), Ctx);
  }

  return "";
}

std::string ASTValue::getValue(const StringLiteral &SL) const {

  auto Result = SL.getBytes().str();
  util::replaceStrAll(Result, "\\", "\\\\");
  util::replaceStrAll(Result, "\'", "\\\'");
  util::replaceStrAll(Result, "\"", "\\\"");
  util::replaceStrAll(Result, "\a", "\\a");
  util::replaceStrAll(Result, "\b", "\\b");
  util::replaceStrAll(Result, "\f", "\\f");
  util::replaceStrAll(Result, "\n", "\\n");
  util::replaceStrAll(Result, "\r", "\\r");
  util::replaceStrAll(Result, "\t", "\\t");
  util::replaceStrAll(Result, "\v", "\\v");

  for (unsigned S = 0; S < Result.size(); ++S) {
    if (Result[S] != '\0')
      continue;
    Result.replace(S, 1, "\\0");
    S += 1;
  }

  return Result;
}

std::vector<ASTValueData>
ASTValue::getDataForArray(const clang::Expr &E,
                          const clang::ASTContext &Ctx) const {

  std::vector<ASTValueData> Result;

  auto *ICE = E.IgnoreCasts();
  if (!ICE)
    return Result;

  auto *ILE = dyn_cast_or_null<InitListExpr>(ICE);
  if (!ILE)
    return Result;

  for (auto *Init : ILE->inits()) {
    if (!Init)
      break;

    auto Data = getData(*Init, Ctx);
    // Only consider one dimensional array
    if (Data.first == true)
      break;
    if (Data.second.size() == 0)
      break;

    Result.push_back(Data.second[0]);
  }

  if (Result.size() != ILE->getNumInits())
    Result.clear();

  return Result;
}

const Expr *ASTValue::getValueExpr(const DeclRefExpr &E,
                                   const ASTContext &Ctx) const {

  auto *TE = const_cast<DeclRefExpr *>(&E);
  auto *ValD = TE->getDecl();
  if (!ValD || !isa<VarDecl>(ValD))
    return nullptr;

  auto &VarD = *dyn_cast<VarDecl>(ValD);

  Expr *Init = VarD.getInit();
  for (auto *ReDecl : VarD.redecls()) {
    auto *NVDInit = ReDecl->getInit();
    if (NVDInit) {
      if (Init)
        break;

      Init = NVDInit;
    }
  }

  return Init;
}

bool ASTValue::isArray() const { return IsArray; }
const std::vector<ASTValueData> &ASTValue::getValues() const { return Values; }

} // end namespace ftg
