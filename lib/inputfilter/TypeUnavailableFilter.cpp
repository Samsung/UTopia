#include "ftg/inputfilter/TypeUnavailableFilter.h"

using namespace clang;

namespace ftg {

const std::string TypeUnavailableFilter::FilterName = "TypeUnavailableFilter";

TypeUnavailableFilter::TypeUnavailableFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool TypeUnavailableFilter::check(const ASTIRNode &Node) const {
  auto &AST = Node.AST;
  auto &Assignee = AST.getAssignee();
  auto *Assigned = AST.getAssigned();

  auto T = Assignee.getType().getTypePtrOrNull();
  if (Assignee.getNodeType() == ASTNode::CALL && Assigned)
    T = Assigned->getType().getTypePtrOrNull();
  if (!T)
    return true;

  while (T != T->getPointeeOrArrayElementType())
    T = T->getPointeeOrArrayElementType();

  Decl *D = nullptr;
  if (T->getAs<clang::TypedefType>()) {
    auto *TT = T->getAs<clang::TypedefType>();
    if (!TT)
      return false;
    D = TT->getDecl();
  } else if (T->getAs<clang::TagType>()) {
    auto *TT = T->getAs<clang::TagType>();
    if (!TT)
      return false;
    D = TT->getDecl();
  }

  if (!D)
    return false;

  auto &SrcManager = AST.getAssignee().getASTUnit().getSourceManager();
  auto FID = SrcManager.getFileID(D->getBeginLoc());
  assert(FID.isValid() && "Unexpected Program State");

  auto DecomposedLoc = SrcManager.getDecomposedIncludedLoc(FID);
  auto &DecomposedFID = DecomposedLoc.first;
  return !DecomposedFID.isValid();
}

} // namespace ftg
