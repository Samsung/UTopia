#ifndef FTG_TYPE_H
#define FTG_TYPE_H

#include "ftg/JsonSerializable.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/GlobalDef.h"
#include "clang/AST/Type.h"
#include "llvm/Support/Casting.h"

#include <set>

namespace ftg {

class Type;
class ArrayInfo;

class Type {
public:
  enum TypeID { TypeID_Pointer, TypeID_Enum, TypeID_Integer, TypeID_Float };
  enum PtrKind {
    PtrKind_Normal,
    PtrKind_Array,
    PtrKind_String /// char/unsigned char pointer/array. Other special char
                   /// types are not included.
  };
  static std::shared_ptr<Type> createCharPointerType();
  static std::shared_ptr<Type> createType(const clang::QualType &T,
                                          const clang::ASTContext &ASTCtx,
                                          llvm::Argument *A = nullptr,
                                          TargetLib *TL = nullptr);

  Type(TypeID ID);
  Type(const Type &Obj) = delete;
  TypeID getKind() const;
  bool isPrimitiveType() const;
  bool isIntegerType() const;
  bool isFloatType() const;
  bool isEnumType() const;
  bool isPointerType() const;
  bool isNormalPtr() const;
  bool isArrayPtr() const;
  bool isStringType() const;
  bool isFixedLengthArrayPtr() const;
  std::string getASTTypeName() const;
  std::string getNameSpace() const;
  const Type *getRealType(bool ArrayElement = false) const;
  size_t getTypeSize() const;
  std::string getTypeName() const;
  const Enum *getGlobalDef() const;
  const ArrayInfo *getArrayInfo() const;
  const Type *getPointeeType() const;
  bool isBoolean() const;
  bool isUnsigned() const;
  bool isAnyCharacter() const;
  void setASTTypeName(std::string ASTTypeName);
  void setNameSpace(std::string NameSpace);
  void setAnyCharacter(bool AnyCharacter);
  void setTypeSize(size_t TypeSize);
  void setTypeName(std::string TypeName);
  void setBoolean(bool Boolean);
  void setGlobalDef(Enum *GlobalDef);
  void setUnsigned(bool Unsigned);
  void setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo);
  void setPointeeType(std::shared_ptr<Type> PointeeType);
  void setPtrKind(PtrKind Kind);

protected:
  class PointerInfo {
  public:
    PointerInfo(PtrKind Kind);
    void updateArrayInfoFromAST(Type *BaseType, const clang::QualType &QT);
    void setPointeeType(std::shared_ptr<Type> PointeeType);
    PtrKind getPtrKind() const;
    void setPtrKind(PtrKind Kind);
    const Type *getPointeeType() const;
    const ArrayInfo *getArrayInfo() const;
    void setArrayInfo(std::shared_ptr<ArrayInfo> ArrInfo);

  private:
    PtrKind Kind;
    std::shared_ptr<Type> PointeeType = nullptr;
    std::shared_ptr<ArrayInfo> ArrInfo = nullptr;
  };

  std::string ASTTypeName;
  TypeID ID;
  std::string NameSpace;
  size_t TypeSize = 0;
  std::string TypeName;
  Enum *GlobalDef = nullptr;
  bool Boolean = false;
  bool Unsigned = false;
  bool AnyCharacter = false;
  std::shared_ptr<PointerInfo> PtrInfo;

  static std::shared_ptr<Type>
  createPrimitiveType(const clang::QualType &T, const clang::ASTContext &Ctx);
  static std::shared_ptr<Type> createEnumType(const clang::QualType &T,
                                              TargetLib *TL);
  static std::shared_ptr<Type> createPointerType(const clang::QualType &T,
                                                 const clang::ASTContext &Ctx,
                                                 llvm::Argument *A,
                                                 TargetLib *TL);
  void updateArrayInfoFromAST(const clang::QualType &QT);
};

class ArrayInfo : public JsonSerializable {
public:
  enum LengthType { UNLIMITED, VARIABLE, FIXED };

  LengthType getLengthType() const;
  size_t getMaxLength() const;
  bool isIncomplete() const;
  void setIncomplete(bool Incomplete);
  void setLengthType(LengthType LengthTy);
  void setMaxLength(size_t MaxLength);

  Json::Value toJson() const override;
  bool fromJson(Json::Value) override;

private:
  LengthType LengthTy = UNLIMITED;
  size_t MaxLength = 0;
  bool Incomplete = false;
};

} // namespace ftg

#endif
