#include "ftg/inputfilter/TypeUnavailableFilter.h"

using namespace clang;

namespace ftg {

const std::string TypeUnavailableFilter::FilterName = "TypeUnavailableFilter";

TypeUnavailableFilter::TypeUnavailableFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool TypeUnavailableFilter::check(const ASTIRNode &Node) const {
  return hasConstRedecls(Node.AST) || isUndefinedType(Node.AST);
}

bool TypeUnavailableFilter::hasConstRedecls(const ASTDefNode &ADN) const {
  auto *D = ADN.getAssignee().getNode().get<VarDecl>();
  if (!D)
    return false;

  if (!D->getType().isConstQualified())
    return false;

  if (!D->isFirstDecl())
    return true;

  return D != D->getMostRecentDecl();
}

bool TypeUnavailableFilter::isUndefinedType(const ASTDefNode &ADN) const {
  auto &Assignee = ADN.getAssignee();
  auto *Assigned = ADN.getAssigned();

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

  auto &SrcManager = ADN.getAssignee().getASTUnit().getSourceManager();
  auto FID = SrcManager.getFileID(D->getBeginLoc());
  assert(FID.isValid() && "Unexpected Program State");

  auto DecomposedLoc = SrcManager.getDecomposedIncludedLoc(FID);
  auto &DecomposedFID = DecomposedLoc.first;
  return !DecomposedFID.isValid();
}

} // namespace ftg
