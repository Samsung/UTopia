#include "ftg/inputfilter/UnsupportTypeFilter.h"

using namespace clang;

namespace ftg {

const std::string UnsupportTypeFilter::FilterName = "UnsupportTypeFilter";

UnsupportTypeFilter::UnsupportTypeFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool UnsupportTypeFilter::check(const ASTIRNode &Node) const {
  const auto *AN = Node.AST.getNodeForType();
  if (!AN)
    return true;

  const auto *T = AN->getType().getTypePtrOrNull();
  if (!T)
    return true;

  if ((T->isIntegerType() && !T->isEnumeralType()) || T->isRealFloatingType())
    return false;
  if (T->isAnyPointerType()) {
    const auto *ElementT = T->getPointeeType().getTypePtr();
    if (!ElementT)
      return true;
    if (ElementT->isAnyCharacterType())
      return false;
    return true;
  }
  if (T->isArrayType()) {
    if (T->isVariableArrayType())
      return true;

    const auto *ArrayT = T->getAsArrayTypeUnsafe();
    if (!ArrayT)
      return true;

    const auto *ElementT = ArrayT->getElementType().getTypePtrOrNull();
    if (!ElementT)
      return true;

    if (ElementT->isAnyCharacterType() || ElementT->isIntegerType() ||
        ElementT->isRealFloatingType())
      return false;
    return true;
  }
  if (T->isEnumeralType()) {
    const auto *D = T->getAsTagDecl();
    if (!D || !D->isExternallyDeclarable())
      return true;
    return false;
  }
  return true;
}

} // namespace ftg
