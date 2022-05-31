#include "ftg/type/GlobalDef.h"
#include "ftg/type/Type.h"

namespace ftg {

GlobalDef::GlobalDef(GlobalDef::TypeID ID) : ID(ID) {}

GlobalDef::TypeID GlobalDef::getKind() const { return ID; }

std::string GlobalDef::getName() const { return Name; }

std::string GlobalDef::getNameDemangled() const { return NameDemangled; }

void GlobalDef::setName(std::string Name) { this->Name = Name; }

void GlobalDef::setNameDemangled(std::string Name) {
  this->NameDemangled = Name;
}

bool Function::classof(const GlobalDef *D) {
  if (!D)
    return false;

  return D->getKind() == GlobalDef::Function;
}

Function::Function() : GlobalDef(GlobalDef::Function) {}

void Function::addParam(std::shared_ptr<Param> Param) {
  Params.push_back(Param);
}

const Param &Function::getParam(unsigned Idx) const {
  assert(Idx < Params.size() && "Unexpected Program State");
  return *Params[Idx];
}

const std::vector<std::shared_ptr<Param>> &Function::getParams() const {
  return Params;
}

bool Function::isVariadic() const { return Variadic; }

void Function::setVariadic(bool Variadic) { this->Variadic = Variadic; }

bool Struct::classof(const GlobalDef *D) {
  if (!D)
    return false;

  return D->getKind() == GlobalDef::Struct;
}

Struct::Struct() : GlobalDef(GlobalDef::Struct) {}

void Struct::addField(std::shared_ptr<Field> Field) { Fields.push_back(Field); }

const Field &Struct::getField(unsigned Idx) const {
  assert(Idx < Fields.size() && "Unexpected Program State");
  return *Fields[Idx];
}

const std::vector<std::shared_ptr<Field>> &Struct::getFields() const {
  return Fields;
}

bool Enum::classof(const GlobalDef *D) {
  if (!D)
    return false;

  return D->getKind() == GlobalDef::Enum;
}

Enum::Enum() : GlobalDef(GlobalDef::Enum) {}

void Enum::addElement(std::shared_ptr<EnumConst> Element) {
  Elements.push_back(Element);
}

const EnumConst &Enum::getElement(unsigned Idx) const {
  assert(Idx < Elements.size() && "Unexpected Program State");
  return *Elements[Idx];
}

const std::vector<std::shared_ptr<EnumConst>> &Enum::getElements() const {
  return Elements;
}

void Enum::setScoped(bool Scoped) { this->Scoped = Scoped; }

void Enum::setScopedUsingClassTag(bool Scoped) {
  this->ScopedUsingClassTag = Scoped;
}

EnumConst::EnumConst(std::string Name, int64_t Value, Enum *Parent)
    : Name(Name), Parent(Parent), Value(Value) {}

std::string EnumConst::getName() const { return Name; }

int64_t EnumConst::getType() const { return Value; }

TypedElem::TypedElem(TypedElem::TypeID ID) : ID(ID) {}

unsigned TypedElem::getIndex() const { return Idx; }

TypedElem::TypeID TypedElem::getKind() const { return ID; }

const Type &TypedElem::getRealType(bool ArrayElement) const {
  return ElemType->getRealType(ArrayElement);
}

const Type &TypedElem::getType() const {
  assert(ElemType && "Unexpected Program State");
  return *ElemType;
}

std::string TypedElem::getVarName() const { return VarName; }

void TypedElem::setIndex(unsigned Idx) { this->Idx = Idx; }

void TypedElem::setType(std::shared_ptr<Type> ElemType) {
  this->ElemType = ElemType;
}

void TypedElem::setVarName(std::string VarName) { this->VarName = VarName; }

bool Param::classof(const TypedElem *T) {
  if (!T)
    return false;

  return T->getKind() == TypedElem::Param;
}

Param::Param(Function *Parent) : TypedElem(TypedElem::Param), Parent(Parent) {}

const Function &Param::getParent() const {
  assert(Parent && "Unexpected Program State");
  return *Parent;
}

size_t Param::getPtrDepth() const { return PtrDepth; }

void Param::setPtrDepth(const Type &T) {
  size_t PtrDepth = 0;
  const Type *CurT = &T;
  while (const auto *PtrTy = llvm::dyn_cast<PointerType>(CurT)) {
    CurT = PtrTy->getPointeeType();
    PtrDepth++;
  }
  this->PtrDepth = PtrDepth;
}

void Param::setPtrDepth(size_t PtrDepth) { this->PtrDepth = PtrDepth; }

bool Field::classof(const TypedElem *T) {
  if (!T)
    return false;

  return T->getKind() == TypedElem::Field;
}

Field::Field(Struct *Parent) : TypedElem(TypedElem::Field), Parent(Parent) {}

const Struct &Field::getParent() const {
  assert(Parent && "Unexpected Program State");
  return *Parent;
}

} // namespace ftg
