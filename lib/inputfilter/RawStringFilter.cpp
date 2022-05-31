#include "ftg/inputfilter/RawStringFilter.h"
#include "ftg/utils/ASTUtil.h"
#include "clang/AST/Expr.h"

using namespace clang;

namespace ftg {

std::string RawStringFilter::FilterName = "RawString";

RawStringFilter::RawStringFilter(std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(RawStringFilter::FilterName, std::move(NextFilter)) {}

bool RawStringFilter::check(const ASTIRNode &Node) const {
  const auto *Assigned = Node.AST.getAssigned();
  if (!Assigned)
    return false;

  const auto *S = Assigned->getNode().get<Stmt>();
  if (!S)
    return false;

  const auto &SrcManager =
      const_cast<ASTNode *>(Assigned)->getASTUnit().getSourceManager();
  auto LangOptions = clang::LangOptions();
  auto Loc = SrcManager.getTopMacroCallerLoc(util::getDebugLoc(*S));

  Token T;
  if (Lexer::getRawToken(Loc, T, SrcManager, LangOptions))
    return false;
  if (!T.is(tok::raw_identifier))
    return false;

  auto NT = Lexer::findNextToken(Loc, SrcManager, LangOptions);
  if (!NT)
    return false;

  auto Spelling = Lexer::getSpelling(*NT, SrcManager, LangOptions);
  return Spelling.find("\"") == 0;
}

} // namespace ftg
