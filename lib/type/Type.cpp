#include "ftg/type/Type.h"
#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/utils/ASTUtil.h"

using namespace llvm;

namespace ftg {

std::shared_ptr<Type> Type::createType(const clang::QualType &T,
                                       const clang::ASTContext &ASTCtx,
                                       TypedElem *ParentD, Type *ParentT,
                                       llvm::Argument *A, TargetLib *TL) {
  std::shared_ptr<Type> Result;
  if (util::isPointerType(T)) {
    Result = PointerType::createType(T, ASTCtx, ParentD, A, TL);
  } else if (util::isDefinedType(T)) {
    Result = DefinedType::createType(T, TL);
  } else if (T->isVoidType()) {
    Result = std::make_shared<VoidType>();
  } else if (util::isPrimitiveType(T)) {
    Result = PrimitiveType::createType(T);
    // TODO: Move to PrimitiveType member as it is unnecessary in other classes.
    // Only TypeSize of PrimitiveTypes is used when generating fuzzers.
    // Sometimes segfault occurs when getting size of unknown types.
    Result->setTypeSize(ASTCtx.getTypeSizeInChars(T).getQuantity());
  } else {
    return std::make_shared<Type>(Type::Unknown);
  }
  Result->setParent(ParentD);
  Result->setParentType(ParentT);

  auto ASTTypeName = T.getAsString();
  if (auto *ET = T->getAs<clang::ElaboratedType>())
    ASTTypeName = ET->desugar().getAsString();
  Result->setASTTypeName(ASTTypeName);
  Result->setNameSpace("");
  return Result;
}

void Type::updateArrayInfoFromAST(Type &T, const clang::QualType &QT) {
  PointerType *CurType = llvm::dyn_cast<PointerType>(&T);
  clang::QualType CurQT = QT;
  while (CurType) {
    if (const auto *CurArrType = util::getAsArrayType(CurQT)) {
      CurType->setPtrKind(PointerType::PtrKind_Array);

      auto ArrInfo = std::make_unique<ArrayInfo>();
      if (auto *CAT = dyn_cast<clang::ConstantArrayType>(CurArrType)) {
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
    if (PointeeQT->isAnyCharacterType()) {
      // FIXME: It is workaround to determine char pointer type as string.
      CurType->setPtrKind(PointerType::PtrKind_String);
      if (!CurType->getArrayInfo())
        CurType->setArrayInfo(std::make_unique<ArrayInfo>());
    }

    CurQT = PointeeQT;
    CurType =
        llvm::dyn_cast_or_null<PointerType>(CurType->getSettablePointeeType());
  }
}

Type::Type(Type::TypeID ID) : ID(ID) {}

std::string Type::getASTTypeName() const {
  std::string ret = ASTTypeName;
  if (auto V = llvm::dyn_cast<IntegerType>(&this->getRealType())) {
    if (V->isBoolean() && ret.find("_Bool") != std::string::npos) {
      ret.replace(ret.find("_Bool"), 5, "bool");
    }
  }

  // Temporary Fix.
  if (ret == "std::vector::size_type") {
    ret = "size_t";
  }
  return ret;
}

Type::TypeID Type::getKind() const { return ID; }

std::string Type::getNameSpace() const { return NameSpace; }

const TypedElem *Type::getParent() const { return Parent; }

const Type *Type::getParentType() const { return ParentType; }

const Type &Type::getPointeeType() const {
  if (const auto *PT = dyn_cast<PointerType>(this)) {
    auto *PointeeType = PT->getPointeeType();
    assert(PointeeType && "Unexpected Program State");
    return *PointeeType;
  }
  return *this;
}

const Type &Type::getRealType(bool ArrayElement) const {
  // TODO: cover multi-dimensional array
  const Type *Result = this;
  while (const auto *CurType = llvm::dyn_cast<PointerType>(Result)) {
    if (!Result->isSinglePtr()) {
      if (ArrayElement && Result->isArrayPtr()) {
        Result = CurType->getPointeeType();
      } else {
        break;
      }
    }
    Result = CurType->getPointeeType();
  }
  assert(Result && "Unexpected Program State");
  return *Result;
}

Type *Type::getSettablePointeeType() {
  auto *PT = dyn_cast<PointerType>(this);
  if (!PT)
    return nullptr;

  return const_cast<Type *>(PT->getPointeeType());
}

size_t Type::getTypeSize() const { return TypeSize; }

bool Type::isArrayPtr() const {
  if (auto V = dyn_cast<PointerType>(this)) {
    return V->getPtrKind() == PointerType::PtrKind_Array;
  }
  return false;
}

bool Type::isFixedLengthArrayPtr() const {
  if (auto V = dyn_cast<PointerType>(this)) {
    if (const auto *ArrInfo = V->getArrayInfo()) {
      return ArrInfo->getLengthType() == ArrayInfo::FIXED;
    }
  }
  return false;
}

bool Type::isSinglePtr() const {
  if (auto V = dyn_cast<PointerType>(this)) {
    return V->getPtrKind() == PointerType::PtrKind_SinglePtr;
  }
  return false;
}

bool Type::isStringType() const {
  if (auto V = dyn_cast<PointerType>(this)) {
    return V->getPtrKind() == PointerType::PtrKind_String;
  }
  return false;
}

bool Type::isVariableLengthArrayPtr() const {
  if (auto V = dyn_cast<PointerType>(this)) {
    if (const auto *ArrInfo = V->getArrayInfo()) {
      return ArrInfo->getLengthType() == ArrayInfo::VARIABLE;
    }
  }
  return false;
}

void Type::setASTTypeName(std::string ASTTypeName) {
  this->ASTTypeName = ASTTypeName;
}

void Type::setNameSpace(std::string NameSpace) { this->NameSpace = NameSpace; }

void Type::setParent(TypedElem *Parent) { this->Parent = Parent; }

void Type::setParentType(Type *ParentType) { this->ParentType = ParentType; }

void Type::setTypeSize(size_t TypeSize) { this->TypeSize = TypeSize; }

bool VoidType::classof(const Type *T) {
  if (!T)
    return false;

  return T->getKind() == Type::Void;
}

VoidType::VoidType() : Type(Type::Void) {}

bool PrimitiveType::classof(const Type *T) {
  if (!T)
    return false;

  return T->getKind() == Type::Primitive;
}

std::shared_ptr<Type> PrimitiveType::createType(const clang::QualType &T) {
  if (T->isIntegerType()) {
    auto Result = std::make_shared<IntegerType>();
    Result->setUnsigned(T->isUnsignedIntegerType());
    Result->setBoolean(T->isBooleanType());
    Result->setAnyCharacter(T->isAnyCharacterType());
    return Result;
  } else if (T->isFloatingType()) {
    return std::make_shared<FloatType>();
  }
  // if(T->isBuiltinType())
  return std::make_shared<Type>(Type::Unknown);
}

PrimitiveType::PrimitiveType(PrimitiveType::TypeID ID)
    : Type(Type::Primitive), PrimitiveID(ID) {}

PrimitiveType::TypeID PrimitiveType::getKind() const { return PrimitiveID; }

bool IntegerType::classof(const Type *T) {
  auto *PT = llvm::dyn_cast_or_null<PrimitiveType>(T);
  if (!PT)
    return false;

  return (T->getKind() == Type::Primitive) &&
         (PT->getKind() == PrimitiveType::Integer);
}

IntegerType::IntegerType() : PrimitiveType(PrimitiveType::Integer) {}

bool IntegerType::isAnyCharacter() const { return AnyCharacter; }

bool IntegerType::isBoolean() const { return Boolean; }

bool IntegerType::isUnsigned() const { return Unsigned; }

void IntegerType::setAnyCharacter(bool AnyCharacter) {
  this->AnyCharacter = AnyCharacter;
}

void IntegerType::setBoolean(bool Boolean) { this->Boolean = Boolean; }

void IntegerType::setUnsigned(bool Unsigned) { this->Unsigned = Unsigned; }

bool FloatType::classof(const Type *T) {
  auto *FT = llvm::dyn_cast_or_null<PrimitiveType>(T);
  if (!FT)
    return false;

  return (T->getKind() == Type::Primitive) &&
         (FT->getKind() == PrimitiveType::Float);
}

FloatType::FloatType() : PrimitiveType(PrimitiveType::Float) {}

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

bool PointerType::classof(const Type *T) {
  if (!T)
    return false;

  return T->getKind() == Type::Pointer;
}

std::shared_ptr<Type> PointerType::createType(const clang::QualType &T,
                                              const clang::ASTContext &ASTCtx,
                                              TypedElem *ParentD,
                                              llvm::Argument *A,
                                              TargetLib *TL) {
  auto Result = std::make_shared<PointerType>();
  if (!Result)
    return nullptr;

  clang::QualType PointeeT = util::getPointeeTy(T);
  Result->setPointeeType(
      Type::createType(PointeeT, ASTCtx, ParentD, Result.get(), A, TL));
  Result->setPtrToVal(llvm::isa<PrimitiveType>(Result->getPointeeType()));
  return Result;
}

PointerType::PointerType() : Type(Type::Pointer) {}

const ArrayInfo *PointerType::getArrayInfo() const { return ArrInfo.get(); }

const Type *PointerType::getPointeeType() const { return PointeeType.get(); }

PointerType::PtrKind PointerType::getPtrKind() const { return Kind; }

bool PointerType::isPtrToVal() const { return PtrToVal; }

void PointerType::setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo) {
  this->ArrInfo = ArrInfo;
}

void PointerType::setPointeeType(std::shared_ptr<Type> PointeeType) {
  this->PointeeType = PointeeType;
}

void PointerType::setPtrKind(PtrKind Kind) { this->Kind = Kind; }

void PointerType::setPtrToVal(bool PtrToVal) { this->PtrToVal = PtrToVal; }

bool DefinedType::classof(const Type *T) {
  if (!T)
    return false;

  return T->getKind() == Type::Defined;
}

std::shared_ptr<Type> DefinedType::createType(const clang::QualType &T,
                                              TargetLib *TL) {
  if (T->isStructureType()) {
    if (util::isCStyleStruct(T)) {
      auto Result = std::make_shared<StructType>();
      clang::TagDecl *TD = T->getAsTagDecl();
      if (!TD)
        return std::make_shared<Type>(Type::Unknown);

      Result->setTypeName(util::getTypeName(*TD));
      Result->setTyped(TD->getTypedefNameForAnonDecl() != nullptr);
      if (TL)
        Result->setGlobalDef(TL->getStruct(Result->getTypeName()));
      return std::move(Result);
    } else {
      return std::make_shared<ClassType>();
    }
  } else if (T->isUnionType()) {
    return std::make_shared<UnionType>();
  } else if (T->isClassType()) {
    return std::make_shared<ClassType>();
  } else if (T->isFunctionType()) {
    return std::make_shared<FunctionType>();
  } else if (T->isEnumeralType()) {
    auto Result = std::make_shared<EnumType>();
    Result->setUnsigned(T->isUnsignedIntegerType());
    Result->setBoolean(T->isBooleanType());

    clang::TagDecl *TD = T->getAsTagDecl();
    // NOTE: Below condition is to handl when tagDecl is nullptr.
    //       Previous code does not define behavior of above case, thus
    //       conservative handling is inserted.
    if (!TD)
      return std::make_shared<Type>(Type::Unknown);

    Result->setTypeName(TD->getQualifiedNameAsString());
    Result->setTyped(TD->getTypedefNameForAnonDecl() != nullptr);
    if (TL)
      Result->setGlobalDef(TL->getEnum(Result->getTypeName()));
    return Result;
  }
  return std::make_shared<Type>(Type::Unknown);
}

DefinedType::DefinedType(TypeID ID) : Type(Type::Defined), DefinedID(ID) {}

DefinedType::TypeID DefinedType::getKind() const { return DefinedID; }

std::string DefinedType::getTypeName() const { return TypeName; }

bool DefinedType::isTyped() const { return Typed; }

void DefinedType::setTyped(bool Typed) { this->Typed = Typed; }

void DefinedType::setTypeName(std::string TypeName) {
  this->TypeName = TypeName;
}

bool StructType::classof(const Type *T) {
  const auto *DT = llvm::dyn_cast_or_null<DefinedType>(T);
  if (!DT)
    return false;

  return (T->getKind() == Type::Defined) &&
         (DT->getKind() == DefinedType::TypeID_Struct);
}

StructType::StructType() : DefinedType(DefinedType::TypeID_Struct) {}

const Struct *StructType::getGlobalDef() const { return GlobalDef; }

void StructType::setGlobalDef(Struct *GlobalDef) {
  this->GlobalDef = GlobalDef;
}

bool UnionType::classof(const Type *T) {
  const auto *DT = llvm::dyn_cast_or_null<DefinedType>(T);
  if (!DT)
    return false;

  return (T->getKind() == Type::Defined) &&
         (DT->getKind() == DefinedType::TypeID_Union);
}

UnionType::UnionType() : DefinedType(DefinedType::TypeID_Union) {}

bool ClassType::classof(const Type *T) {
  const auto *DT = llvm::dyn_cast_or_null<DefinedType>(T);
  if (!DT)
    return false;

  return (T->getKind() == Type::Defined) &&
         (DT->getKind() == DefinedType::TypeID_Class);
}

ClassType::ClassType() : DefinedType(DefinedType::TypeID_Class) {}

bool ClassType::isStdStringType() const {
  return !util::regex(ASTTypeName, "std::.*string").empty();
}

bool FunctionType::classof(const Type *T) {
  const auto *DT = llvm::dyn_cast_or_null<DefinedType>(T);
  if (!DT)
    return false;

  return (T->getKind() == Type::Defined) &&
         (DT->getKind() == DefinedType::TypeID_Function);
}

FunctionType::FunctionType() : DefinedType(DefinedType::TypeID_Function) {}

bool EnumType::classof(const Type *T) {
  const auto *DT = llvm::dyn_cast_or_null<DefinedType>(T);
  if (!DT)
    return false;

  return (T->getKind() == Type::Defined) &&
         (DT->getKind() == DefinedType::TypeID_Enum);
}

EnumType::EnumType() : DefinedType(DefinedType::TypeID_Enum) {}

const Enum *EnumType::getGlobalDef() const { return GlobalDef; }

bool EnumType::isBoolean() const { return Boolean; }

bool EnumType::isUnsigned() const { return Unsigned; }

void EnumType::setBoolean(bool Boolean) { this->Boolean = Boolean; }

void EnumType::setGlobalDef(Enum *GlobalDef) { this->GlobalDef = GlobalDef; }

void EnumType::setUnsigned(bool Unsigned) { this->Unsigned = Unsigned; }

} // namespace ftg
