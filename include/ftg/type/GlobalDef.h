#ifndef FTG_GLOBAL_DEF_H
#define FTG_GLOBAL_DEF_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ftg {
// Nodes
class Const;
class Type;

// GlobalDefs
class GlobalDef;
class Function;
class Struct;
class Enum;

// TypedElems
class TypedElem;
class Param;
class Field;

class EnumConst;

// GlobalDef

class GlobalDef {
public:
  enum TypeID {
    Function,
    Struct,
    Union,
    Class,
    Enum,
    // Interface
  };
  GlobalDef(TypeID ID);
  TypeID getKind() const;
  std::string getName() const;
  std::string getNameDemangled() const;
  void setName(std::string Name);
  void setNameDemangled(std::string NameDemangled);

protected:
  TypeID ID;
  std::string Name;          // mangled - unique name
  std::string NameDemangled; // demangled - human readable name
};

class Function : public GlobalDef {
public:
  static bool classof(const GlobalDef *D);
  Function();
  void addParam(std::shared_ptr<Param> Param);
  const Param &getParam(unsigned Idx) const;
  const std::vector<std::shared_ptr<Param>> &getParams() const;
  bool isVariadic() const;
  void setVariadic(bool Variadic);

private:
  // std::shared_ptr<Type> retType; // TODO: implement
  std::vector<std::shared_ptr<Param>> Params;
  bool Variadic = false;
};

class Struct : public GlobalDef {
public:
  static bool classof(const GlobalDef *D);
  Struct();
  void addField(std::shared_ptr<Field> Field);
  const Field &getField(unsigned Index) const;
  const std::vector<std::shared_ptr<Field>> &getFields() const;

private:
  std::vector<std::shared_ptr<Field>> Fields;
};

class Enum : public GlobalDef {
public:
  static bool classof(const GlobalDef *D);
  Enum();
  void addElement(std::shared_ptr<EnumConst> Element);
  const EnumConst &getElement(unsigned Idx) const;
  const std::vector<std::shared_ptr<EnumConst>> &getElements() const;
  void setScoped(bool Scoped);
  void setScopedUsingClassTag(bool Scoped);

private:
  std::vector<std::shared_ptr<EnumConst>> Elements;
  bool Scoped = false;
  bool ScopedUsingClassTag = false;
};

class EnumConst { // : public Const
public:
  EnumConst(std::string Name, int64_t Value, Enum *Parent);
  std::string getName() const;
  int64_t getType() const;

protected:
  std::string Name;
  Enum *Parent;
  int64_t Value;
};

class TypedElem {
public:
  enum TypeID {
    Param,
    Field,
  };
  TypedElem(TypeID ID);
  unsigned getIndex() const;
  TypeID getKind() const;
  const Type &getRealType(bool ArrayElement = false) const;
  const Type &getType() const;
  std::string getVarName() const;
  void setIndex(unsigned Idx);
  void setType(std::shared_ptr<Type> ElemType);
  void setVarName(std::string VarName);

protected:
  TypeID ID;
  std::shared_ptr<Type> ElemType;
  unsigned Idx = 0;
  std::string VarName;
};

class Param : public TypedElem {
public:
  static bool classof(const TypedElem *T);
  Param(Function *Parent);
  const Function &getParent() const;
  size_t getPtrDepth() const;
  void setPtrDepth(const Type &T);
  void setPtrDepth(size_t PtrDepth);

private:
  Function *Parent;
  size_t PtrDepth = 0;
};

class Field : public TypedElem {
public:
  static bool classof(const TypedElem *T);
  Field(Struct *Parent);
  const Struct &getParent() const;

private:
  Struct *Parent;
};

} // namespace ftg

#endif
