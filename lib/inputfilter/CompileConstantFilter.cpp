#include "ftg/inputfilter/CompileConstantFilter.h"
#include "ftg/utils/ASTUtil.h"

using namespace clang;

namespace ftg {

const std::string CompileConstantFilter::FilterName = "CompileConstantFilter";

CompileConstantFilter::CompileConstantFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool CompileConstantFilter::check(const ASTIRNode &Node) const {
  return isMacroFunctionAssigned(Node.AST) || isOffsetOfExpr(Node.AST);
}

bool CompileConstantFilter::isMacroFunctionAssigned(
    const ASTDefNode &Node) const {
  const auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return false;

  const auto *S = Assigned->getNode().get<Stmt>();
  if (!S)
    return false;

  const auto &SrcManager =
      const_cast<ASTNode *>(Assigned)->getASTUnit().getSourceManager();
  auto Loc = SrcManager.getTopMacroCallerLoc(S->getBeginLoc());
  return util::getMacroFunctionExpansionRange(SrcManager, Loc).isValid();
}

bool CompileConstantFilter::isOffsetOfExpr(const ASTDefNode &Node) const {
  const auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return false;

  const auto *E = dyn_cast_or_null<Expr>(Assigned->getNode().get<Expr>());
  if (!E)
    return false;

  return dyn_cast_or_null<OffsetOfExpr>(E->IgnoreCasts());
}

} // namespace ftg
