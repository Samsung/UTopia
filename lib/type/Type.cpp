#include "ftg/type/Type.h"
#include "ftg/utils/ASTUtil.h"
#include "json/json.h"

using namespace llvm;

namespace ftg {

std::shared_ptr<Type> Type::createCharPointerType() {
  auto CharType = std::make_shared<Type>(Type::TypeID_Integer);
  if (!CharType)
    return nullptr;
  CharType->setAnyCharacter(true);
  CharType->setASTTypeName("char");
  CharType->setTypeSize(1);

  auto CharPtrType = std::make_shared<Type>(Type::TypeID_Pointer);
  CharPtrType->setPointeeType(CharType);
  CharPtrType->setASTTypeName("char *");
  CharPtrType->setPtrKind(Type::PtrKind_String);
  return CharPtrType;
}

std::shared_ptr<Type> Type::createType(const clang::QualType &T,
                                       const clang::ASTContext &ASTCtx,
                                       llvm::Argument *A,
                                       const TypeAnalysisReport *Report) {
  std::shared_ptr<Type> Result = nullptr;
  if (util::isPointerType(T)) {
    Result = Type::createPointerType(T, ASTCtx, A, Report);
  } else if (T->isEnumeralType()) {
    Result = Type::createEnumType(T, ASTCtx, Report);
  } else if (util::isPrimitiveType(T)) {
    Result = Type::createPrimitiveType(T, ASTCtx);
  }
  if (!Result)
    return nullptr;

  // Follow policy of target source but set Bool 1 to use 'bool'
  auto PP = ASTCtx.getPrintingPolicy();
  PP.Bool = 1;
  auto ASTTypeName = T.getAsString(PP);
  if (const auto *ET = T->getAs<clang::ElaboratedType>())
    ASTTypeName = ET->desugar().getAsString(PP);
  Result->setASTTypeName(ASTTypeName);
  Result->setNameSpace("");
  return Result;
}

std::shared_ptr<Type> Type::createPrimitiveType(const clang::QualType &T,
                                                const clang::ASTContext &Ctx) {
  // Only TypeSize of PrimitiveTypes is used when generating fuzzers.
  // Sometimes segfault occurs when getting size of unknown types.
  if (T->isIntegerType()) {
    auto Result = std::make_shared<Type>(Type::TypeID_Integer);
    Result->setUnsigned(T->isUnsignedIntegerType());
    Result->setBoolean(T->isBooleanType());
    Result->setAnyCharacter(T->isAnyCharacterType());
    Result->setTypeSize(Ctx.getTypeSizeInChars(T).getQuantity());
    return Result;
  } else if (T->isFloatingType()) {
    auto Result = std::make_shared<Type>(Type::TypeID_Float);
    Result->setTypeSize(Ctx.getTypeSizeInChars(T).getQuantity());
    return Result;
  }
  return nullptr;
}

std::shared_ptr<Type> Type::createEnumType(const clang::QualType &T,
                                           const clang::ASTContext &Ctx,
                                           const TypeAnalysisReport *Report) {
  if (!T->isEnumeralType())
    return nullptr;

  auto *D = T->getAsTagDecl();
  if (!D)
    return nullptr;

  auto Result = std::make_shared<Type>(Type::TypeID_Enum);
  Result->setUnsigned(T->isUnsignedIntegerType());
  Result->setBoolean(T->isBooleanType());
  auto EnumName = D->getQualifiedNameAsString();
  if (auto *TD = D->getTypedefNameForAnonDecl())
    EnumName = TD->getQualifiedNameAsString();
  Result->setTypeName(EnumName);
  if (Report) {
    const auto &Enums = Report->getEnums();
    auto Iter = Enums.find(EnumName);
    if (Iter != Enums.end())
      Result->setGlobalDef(Iter->second);
  }
  return Result;
}

std::shared_ptr<Type>
Type::createPointerType(const clang::QualType &T,
                        const clang::ASTContext &ASTCtx, llvm::Argument *A,
                        const TypeAnalysisReport *Report) {
  auto Result = std::make_shared<Type>(Type::TypeID_Pointer);
  if (!Result)
    return nullptr;

  Result->setPtrKind(Type::PtrKind::PtrKind_Normal);
  clang::QualType PointeeT = util::getPointeeTy(T);
  Result->setPointeeType(Type::createType(PointeeT, ASTCtx, A, Report));
  Result->updateArrayInfoFromAST(T);
  return Result;
}

void Type::updateArrayInfoFromAST(const clang::QualType &QT) {
  assert(PtrInfo && "Unexpected Program State");
  PtrInfo->updateArrayInfoFromAST(this, QT);
}

Type::Type(Type::TypeID ID) : ID(ID) {
  if (ID == TypeID_Pointer)
    this->PtrInfo =
        std::make_shared<PointerInfo>(Type::PtrKind::PtrKind_Normal);
}

Type::Type(Json::Value Json) {
  assert(fromJson(Json) && "Unexpected Program State");
}

Type::Type(Json::Value Json, const TypeAnalysisReport *Report) {
  assert(fromJson(Json, Report) && "Unexpected Program State");
}

Type::TypeID Type::getKind() const { return ID; }

bool Type::isPrimitiveType() const { return isIntegerType() || isFloatType(); }

bool Type::isIntegerType() const { return ID == Type::TypeID_Integer; }

bool Type::isFloatType() const { return ID == Type::TypeID_Float; }

bool Type::isEnumType() const { return ID == Type::TypeID_Enum; }

bool Type::isPointerType() const { return ID == Type::TypeID_Pointer; }

bool Type::isNormalPtr() const {
  return PtrInfo && PtrInfo->getPtrKind() == Type::PtrKind_Normal;
}

bool Type::isArrayPtr() const {
  return PtrInfo && PtrInfo->getPtrKind() == Type::PtrKind_Array;
}

bool Type::isStringType() const {
  return PtrInfo && PtrInfo->getPtrKind() == Type::PtrKind_String;
}

bool Type::isFixedLengthArrayPtr() const {
  if (this->isPointerType() && PtrInfo) {
    if (const auto *ArrInfo = this->getArrayInfo()) {
      return ArrInfo->getLengthType() == ArrayInfo::FIXED;
    }
  }
  return false;
}

std::string Type::getASTTypeName() const {
  std::string Ret = ASTTypeName;

  // Temporary Fix.
  if (Ret == "std::vector::size_type") {
    Ret = "size_t";
  }
  return Ret;
}

std::string Type::getNameSpace() const { return NameSpace; }

const Type *Type::getRealType(bool ArrayElement) const {
  // TODO: cover multi-dimensional array
  const Type *Result = this;
  while (Result && Result->isPointerType()) {
    if (Result->isNormalPtr()) {
      Result = Result->getPointeeType();
      continue;
    }

    if (ArrayElement && Result->isArrayPtr()) {
      Result = Result->getPointeeType();
    } else {
      break;
    }
  }
  return Result;
}

size_t Type::getTypeSize() const { return TypeSize; }

std::string Type::getTypeName() const { return TypeName; }

const Enum *Type::getGlobalDef() const { return GlobalDef.get(); }

const ArrayInfo *Type::getArrayInfo() const {
  assert(PtrInfo && "Unexpected Program State");
  return PtrInfo->getArrayInfo();
}

const Type *Type::getPointeeType() const {
  if (isPointerType()) {
    assert(PtrInfo && "Unexpected Program State");
    return PtrInfo->getPointeeType();
  }
  return this;
}

bool Type::isBoolean() const { return Boolean; }

bool Type::isUnsigned() const { return Unsigned; }

bool Type::isAnyCharacter() const { return AnyCharacter; }

void Type::setASTTypeName(std::string ASTTypeName) {
  this->ASTTypeName = ASTTypeName;
}

void Type::setNameSpace(std::string NameSpace) { this->NameSpace = NameSpace; }

void Type::setAnyCharacter(bool AnyCharacter) {
  this->AnyCharacter = AnyCharacter;
}

void Type::setTypeSize(size_t TypeSize) { this->TypeSize = TypeSize; }

void Type::setTypeName(std::string TypeName) { this->TypeName = TypeName; }

void Type::setBoolean(bool Boolean) { this->Boolean = Boolean; }

void Type::setGlobalDef(std::shared_ptr<Enum> GlobalDef) {
  this->GlobalDef = GlobalDef;
}

void Type::setUnsigned(bool Unsigned) { this->Unsigned = Unsigned; }

void Type::setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo) {
  assert(PtrInfo && "Unexpected Program State");
  this->PtrInfo->setArrayInfo(ArrInfo);
}

void Type::setPointeeType(std::shared_ptr<Type> PointeeType) {
  assert(PtrInfo && "Unexpected Program State");
  this->PtrInfo->setPointeeType(PointeeType);
}

void Type::setPtrKind(Type::PtrKind Kind) {
  assert(PtrInfo && "Unexpected Program State");
  this->PtrInfo->setPtrKind(Kind);
}

Json::Value Type::toJson() const {
  Json::Value Json;

  Json["ASTTypeName"] = ASTTypeName;
  Json["TypeID"] = ID;
  Json["NameSpace"] = NameSpace;
  Json["TypeSize"] = TypeSize;
  Json["TypeName"] = TypeName;
  Json["GlobalDef"] = GlobalDef ? GlobalDef->toJson() : Json::nullValue;
  Json["IsBoolean"] = Boolean;
  Json["IsUnsigned"] = Unsigned;
  Json["IsAnyCharacter"] = AnyCharacter;
  Json["PointerInfo"] = PtrInfo ? PtrInfo->toJson() : Json::nullValue;

  return Json;
}

bool Type::fromJson(Json::Value Json) { return Type::fromJson(Json, nullptr); }

bool Type::fromJson(Json::Value Json, const TypeAnalysisReport *Report) {
  try {
    if (Json.isNull() || Json.empty() || !Json["ASTTypeName"].isString() ||
        !Json["TypeID"].isUInt() || !Json["NameSpace"].isString() ||
        !Json["TypeSize"].isUInt() || !Json["TypeName"].isString() ||
        !Json["IsBoolean"].isBool() || !Json["IsUnsigned"].isBool() ||
        !Json["IsAnyCharacter"].isBool())
      throw Json::LogicError("Abnormal Json Value");
    ASTTypeName = Json["ASTTypeName"].asString();
    ID = static_cast<TypeID>(Json["TypeID"].asUInt());
    NameSpace = Json["NameSpace"].asString();
    TypeSize = Json["TypeSize"].asUInt();
    TypeName = Json["TypeName"].asString();
    GlobalDef = nullptr;
    if (Report) {
      const auto &Enums = Report->getEnums();
      auto Iter = Enums.find(TypeName);
      if (Iter != Enums.end())
        GlobalDef = Iter->second;
    }
    Boolean = Json["IsBoolean"].asBool();
    Unsigned = Json["IsUnsigned"].asBool();
    AnyCharacter = Json["IsAnyCharacter"].asBool();
    PtrInfo = Json["PointerInfo"].isNull()
                  ? nullptr
                  : std::make_shared<PointerInfo>(Json["PointerInfo"], Report);
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

ArrayInfo::LengthType ArrayInfo::getLengthType() const { return LengthTy; }

size_t ArrayInfo::getMaxLength() const { return MaxLength; }

bool ArrayInfo::isIncomplete() const { return Incomplete; }

void ArrayInfo::setIncomplete(bool Incomplete) {
  this->Incomplete = Incomplete;
}

void ArrayInfo::setLengthType(LengthType LengthTy) {
  this->LengthTy = LengthTy;
}

void ArrayInfo::setMaxLength(size_t MaxLength) { this->MaxLength = MaxLength; }

Type::PointerInfo::PointerInfo(PtrKind Kind) : Kind(Kind) {}

Type::PointerInfo::PointerInfo(Json::Value Json) {
  assert(fromJson(Json) && "Unexpected Program State");
}

Type::PointerInfo::PointerInfo(Json::Value Json,
                               const TypeAnalysisReport *Report) {
  assert(fromJson(Json, Report) && "Unexpected Program State");
}

void Type::PointerInfo::updateArrayInfoFromAST(Type *BaseType,
                                               const clang::QualType &QT) {
  auto *CurType = BaseType;
  clang::QualType CurQT = QT;
  while (CurType && CurType->isPointerType()) {
    if (const auto *CurArrType = util::getAsArrayType(CurQT)) {
      CurType->setPtrKind(PtrKind::PtrKind_Array);

      auto ArrInfo = std::make_unique<ArrayInfo>();
      if (const auto *CAT = dyn_cast<clang::ConstantArrayType>(CurArrType)) {
        ArrInfo->setLengthType(ArrayInfo::FIXED);
        ArrInfo->setMaxLength(CAT->getSize().getZExtValue());
      } else if (isa<clang::VariableArrayType>(CurArrType)) {
        ArrInfo->setLengthType(ArrayInfo::VARIABLE);
      } else if (CurArrType->isIncompleteArrayType()) {
        ArrInfo->setIncomplete(true);
      }
      CurType->setArrayInfo(std::move(ArrInfo));
    }

    auto PointeeQT = util::getPointeeTy(CurQT);
    if (PointeeQT->isCharType()) {
      // FIXME: It is workaround to determine char pointer type as string.
      CurType->setPtrKind(PtrKind::PtrKind_String);
      if (!CurType->getArrayInfo()) {
        CurType->setArrayInfo(std::make_unique<ArrayInfo>());
      }
    }

    CurQT = PointeeQT;
    CurType = const_cast<Type *>(CurType->getPointeeType());
  }
}

void Type::PointerInfo::setPointeeType(std::shared_ptr<Type> PointeeType) {
  this->PointeeType = PointeeType;
}

Type::PtrKind Type::PointerInfo::getPtrKind() const { return Kind; }

void Type::PointerInfo::setPtrKind(PtrKind Kind) { this->Kind = Kind; }

const Type *Type::PointerInfo::getPointeeType() const {
  return PointeeType.get();
}

const ArrayInfo *Type::PointerInfo::getArrayInfo() const {
  return ArrInfo.get();
}

void Type::PointerInfo::setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo) {
  this->ArrInfo = ArrInfo;
}

Json::Value Type::PointerInfo::toJson() const {
  Json::Value Json;

  Json["PointerKind"] = Kind;
  Json["PointeeType"] = PointeeType ? PointeeType->toJson() : Json::nullValue;
  Json["ArrayInfo"] = ArrInfo ? ArrInfo->toJson() : Json::nullValue;

  return Json;
}

bool Type::PointerInfo::fromJson(Json::Value Json) {
  return fromJson(Json, nullptr);
}

bool Type::PointerInfo::fromJson(Json::Value Json,
                                 const TypeAnalysisReport *Report) {
  try {
    if (Json.isNull() || Json.empty() || !Json["PointerKind"].isInt())
      throw Json::LogicError("Abnormal Json Value");
    Kind = static_cast<PtrKind>(Json["PointerKind"].asInt());
    if (!Json["PointeeType"].isNull()) {
      PointeeType = std::make_shared<Type>(Json["PointeeType"], Report);
    }
    if (!Json["ArrayInfo"].isNull()) {
      ArrInfo = std::make_unique<ArrayInfo>();
      ArrInfo->fromJson(Json["ArrayInfo"]);
    }
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

Json::Value ArrayInfo::toJson() const {
  Json::Value Json;

  Json["LengthType"] = LengthTy;
  Json["MaxLength"] = MaxLength;
  Json["Incomplete"] = Incomplete;

  return Json;
}

bool ArrayInfo::fromJson(Json::Value Json) {
  try {
    if (Json.isNull() || Json.empty() || !Json["LengthType"].isInt() ||
        !Json["MaxLength"].isUInt64() || !Json["Incomplete"].isBool())
      throw Json::LogicError("Abnormal Json Value");
    LengthTy = static_cast<LengthType>(Json["LengthType"].asInt());
    MaxLength = Json["MaxLength"].asUInt64();
    Incomplete = Json["Incomplete"].asBool();
  } catch (Json::LogicError &E) {
    llvm::outs() << "[E] " << E.what() << "\n";
    return false;
  }
  return true;
}

} // namespace ftg
