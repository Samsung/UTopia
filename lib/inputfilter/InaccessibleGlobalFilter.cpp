#include "ftg/inputfilter/InaccessibleGlobalFilter.h"

using namespace ftg;
using namespace clang;

const std::string InaccessibleGlobalFilter::FilterName =
    "InaccessibleGlobalFilter";

InaccessibleGlobalFilter::InaccessibleGlobalFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool InaccessibleGlobalFilter::check(const ASTIRNode &Node) const {
  if (isNonAccessibleInitializer(Node.AST))
    return true;
  if (isNonPublicStaticMember(Node.AST))
    return true;
  return false;
}

bool InaccessibleGlobalFilter::isNonAccessibleInitializer(
    ASTDefNode &Node) const {
  const auto *AST = Node.getAssignee().getNode().get<CXXCtorInitializer>();
  if (!AST)
    return false;
  return AST->isInClassMemberInitializer();
}

bool InaccessibleGlobalFilter::isNonPublicStaticMember(ASTDefNode &Node) const {
  const auto *D = Node.getAssignee().getNode().get<VarDecl>();
  if (!D)
    return false;

  auto Storage = D->getStorageClass();
  auto Access = D->getAccess();
  if (Storage != SC_Static || (Access != AS_protected && Access != AS_private))
    return false;

  return true;
}
