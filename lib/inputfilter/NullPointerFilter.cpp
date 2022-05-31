#include "ftg/inputfilter/NullPointerFilter.h"
#include "clang/AST/Expr.h"

using namespace clang;

namespace ftg {

const std::string NullPointerFilter::FilterName = "NullPointerFilter";

NullPointerFilter::NullPointerFilter(std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool NullPointerFilter::check(const ASTIRNode &Node) const {
  auto *Assigned = Node.AST.getAssigned();
  if (!Assigned)
    return false;

  auto *E = Assigned->getNode().get<Expr>();
  if (!E)
    return false;

  auto NullValue = E->isNullPointerConstant(
      Assigned->getASTUnit().getASTContext(),
      clang::AbstractConditionalOperator::NPC_NeverValueDependent);
  if (NullValue == clang::AbstractConditionalOperator::NPCK_NotNull)
    return false;

  const clang::Type *T = nullptr;
  const auto &Assignee = Node.AST.getAssignee();
  if (Assignee.getNodeType() == ASTNode::CALL)
    T = Assigned->getType().getTypePtrOrNull();
  else
    T = Node.AST.getAssignee().getType().getTypePtrOrNull();

  if (!T)
    return false;

  return T->isAnyPointerType();
}

} // namespace ftg
