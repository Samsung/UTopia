#ifndef FTG_TYPE_H
#define FTG_TYPE_H

#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/GlobalDef.h"
#include "clang/AST/Type.h"
#include "llvm/Support/Casting.h"

#include <set>

namespace ftg {

class Type;

// Types
class VoidType;
class PrimitiveType;
class PointerType;
class DefinedType;

// PrimitiveTypes
class IntegerType;
class FloatType;

// PointerTypes
class ArrayInfo;

// DefinedTypes
class StructType;
class UnionType;
class ClassType;
class FunctionType;

class Type {
public:
  enum TypeID { Void, Primitive, Pointer, Defined, Unknown };

  static std::shared_ptr<Type>
  createType(const clang::QualType &T, const clang::ASTContext &ASTCtx,
             TypedElem *ParentD = nullptr, Type *ParentT = nullptr,
             llvm::Argument *A = nullptr, TargetLib *TL = nullptr);
  static void updateArrayInfoFromAST(Type &T, const clang::QualType &QT);

  Type(TypeID ID);
  Type(const Type &Obj) = delete;
  std::string getASTTypeName() const;
  TypeID getKind() const;
  std::string getNameSpace() const;
  const TypedElem *getParent() const;
  const Type *getParentType() const;
  const Type &getPointeeType() const;
  const Type &getRealType(bool ArrayElement = false) const;
  Type *getSettablePointeeType();
  size_t getTypeSize() const;
  bool isArrayPtr() const;
  bool isFixedLengthArrayPtr() const;
  bool isSinglePtr() const;
  bool isStringType() const;
  bool isVariableLengthArrayPtr() const;
  void setASTTypeName(std::string ASTTypeName);
  void setNameSpace(std::string NameSpace);
  void setParent(TypedElem *Parent);
  void setParentType(Type *ParentType);
  void setTypeSize(size_t typeSize);

protected:
  std::string ASTTypeName;
  TypeID ID;
  std::string NameSpace;
  TypedElem *Parent = nullptr;
  Type *ParentType = nullptr;
  size_t TypeSize = 0;
};

class VoidType : public Type {
public:
  static bool classof(const Type *V);
  VoidType();
};

class PrimitiveType : public Type {
public:
  enum TypeID { Integer, Float };
  static bool classof(const Type *T);
  static std::shared_ptr<Type> createType(const clang::QualType &T);
  PrimitiveType(TypeID typeID);
  TypeID getKind() const;

private:
  TypeID PrimitiveID;
};

class IntegerType : public PrimitiveType {
public:
  static bool classof(const Type *T);
  IntegerType();
  bool isAnyCharacter() const;
  bool isBoolean() const;
  bool isUnsigned() const;
  void setAnyCharacter(bool AnyCharacter);
  void setBoolean(bool Boolean);
  void setUnsigned(bool Unsigned);

private:
  bool Boolean = false;
  bool AnyCharacter = false;
  bool Unsigned = false;
};

class FloatType : public PrimitiveType {
public:
  static bool classof(const Type *T);
  FloatType();
};

class ArrayInfo {
public:
  enum LengthType { UNLIMITED, VARIABLE, FIXED };

  LengthType getLengthType() const;
  size_t getMaxLength() const;
  bool isIncomplete() const;
  void setIncomplete(bool Incomplete);
  void setLengthType(LengthType LengthTy);
  void setMaxLength(size_t MaxLength);

private:
  LengthType LengthTy = UNLIMITED;
  size_t MaxLength = 0;
  bool Incomplete = false;
};

class PointerType : public Type {
public:
  enum PtrKind {
    PtrKind_SinglePtr,
    PtrKind_Array,
    PtrKind_Buffer,
    PtrKind_String
  };

  static bool classof(const Type *V);
  static std::shared_ptr<Type> createType(const clang::QualType &T,
                                          const clang::ASTContext &Ctx,
                                          TypedElem *ParentD, llvm::Argument *A,
                                          TargetLib *TL);
  PointerType();
  const ArrayInfo *getArrayInfo() const;
  const Type *getPointeeType() const;
  PtrKind getPtrKind() const;
  bool isPtrToVal() const;
  void setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo);
  void setPointeeType(std::shared_ptr<Type> PointeeType);
  void setPtrKind(PtrKind Kind);
  void setPtrToVal(bool PtrToVal);

private:
  PtrKind Kind = PtrKind_SinglePtr;
  std::shared_ptr<Type> PointeeType;
  std::shared_ptr<ArrayInfo> ArrInfo;
  bool PtrToVal = false;
};

class DefinedType : public Type {
public:
  enum TypeID {
    TypeID_Struct,
    TypeID_Union,
    TypeID_Class,
    TypeID_Function,
    TypeID_Enum,
  };

  static bool classof(const Type *V);
  static std::shared_ptr<Type> createType(const clang::QualType &T,
                                          TargetLib *TL);
  DefinedType(TypeID ID);
  TypeID getKind() const;
  std::string getTypeName() const;
  bool isTyped() const;
  void setTyped(bool Typed);
  void setTypeName(std::string TypeName);

protected:
  TypeID DefinedID;
  std::string TypeName;
  bool Typed = false;
};

class StructType : public DefinedType {
public:
  static bool classof(const Type *T);
  StructType();
  const Struct *getGlobalDef() const;
  void setGlobalDef(Struct *GlobalDef);

private:
  Struct *GlobalDef = nullptr;
};

class UnionType : public DefinedType {
public:
  static bool classof(const Type *T);
  UnionType();
};

class ClassType : public DefinedType {
public:
  static bool classof(const Type *T);
  ClassType();
  bool isStdStringType() const;
};

class FunctionType : public DefinedType {
public:
  static bool classof(const Type *T);
  FunctionType();
};

class EnumType : public DefinedType {
public:
  static bool classof(const Type *T);
  EnumType();
  const Enum *getGlobalDef() const;
  bool isBoolean() const;
  bool isUnsigned() const;
  void setBoolean(bool Boolean);
  void setGlobalDef(Enum *GlobalDef);
  void setUnsigned(bool Unsigned);

private:
  Enum *GlobalDef = nullptr;
  bool Boolean = false;
  bool Unsigned = false;
};

} // namespace ftg

#endif
