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
    Enumerators.emplace_back(
        EnumConst(Const->getNameAsString(), Const->getInitVal().getExtValue()));
  }
}

std::string Enum::getName() const { return Name; }

const std::vector<EnumConst> &Enum::getElements() const { return Enumerators; }

EnumConst::EnumConst(std::string Name, int64_t Value)
    : Name(Name), Value(Value) {}

std::string EnumConst::getName() const { return Name; }

int64_t EnumConst::getValue() const { return Value; }

} // namespace ftg
