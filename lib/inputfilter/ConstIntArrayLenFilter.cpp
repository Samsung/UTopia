#include "ftg/inputfilter/ConstIntArrayLenFilter.h"
#include "ftg/utils/LLVMUtil.h"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

const std::string ConstIntArrayLenFilter::FilterName = "ConstIntArrayLenFilter";

ConstIntArrayLenFilter::ConstIntArrayLenFilter(
    std::unique_ptr<InputFilter> NextFilter)
    : InputFilter(FilterName, std::move(NextFilter)) {}

bool ConstIntArrayLenFilter::check(const ASTIRNode &Node) const {
  auto *D = Node.AST.getAssignee().getNode().get<VarDecl>();
  if (!D)
    return false;

  auto &Ctx = D->getASTContext();
  auto QT = D->getType();
  auto *T = QT.getTypePtrOrNull();
  if (!QT.isConstant(Ctx) || !T || !T->isIntegerType())
    return false;

  Expr::EvalResult Eval;
  auto *Init = D->getInit();
  auto VarName = D->getNameAsString();
  if (!Init || !Init->EvaluateAsInt(Eval, Ctx) || VarName.empty())
    return false;

  auto Value = Eval.Val.getInt().getExtValue();
  if (Value < 0)
    return false;

  auto LangOptions = clang::LangOptions();
  auto &SrcManager = Ctx.getSourceManager();
  const std::string Tag = "Tag";
  auto Matcher = varDecl(hasType(constantArrayType())).bind(Tag);
  for (auto &Node : match(Matcher, *const_cast<ASTContext *>(&Ctx))) {
    auto *Record = Node.getNodeAs<VarDecl>(Tag);
    if (!Record)
      continue;

    auto Loc = Record->getLocation();
    auto EndLoc = Record->getEndLoc();
    auto Offset = SrcManager.getFileOffset(Loc);
    int Length = SrcManager.getFileOffset(EndLoc) - Offset;
    if (Length < 0)
      continue;

    Length += Lexer::MeasureTokenLength(EndLoc, SrcManager, LangOptions);
    auto Content = util::getFileContent(SrcManager.getFileID(Loc), SrcManager);
    if (Content.size() < Offset + Length)
      continue;

    auto Data = std::string(Content.substr(Offset, Length));
    Data = Data.substr(Data.find("["), Data.find("="));
    if (Data.find(VarName) == std::string::npos)
      continue;

    return true;
  }
  return false;
}
