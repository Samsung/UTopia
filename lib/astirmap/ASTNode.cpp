#include "ftg/astirmap/ASTNode.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/FileUtil.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Attr.h"
#include "clang/AST/ExprCXX.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace ast_type_traits;

namespace ftg {

ASTNode::ASTNode(nodeType Type, const DynTypedNode Node, ASTUnit &Unit)
    : Node(Node), NodeType(Type), Unit(Unit) {
  Ty = getType(Node);
  const auto &SrcManager = Unit.getSourceManager();
  auto BeginLoc = getBeginLoc(Node, SrcManager);
  assert(BeginLoc.isValid() && "Unexpected Program State");
  Offset = SrcManager.getDecomposedExpansionLoc(BeginLoc).second;
  std::tie(Length, Offset) = getLengthAndOffset(Node, BeginLoc, SrcManager);

  auto ExpandedBeginLoc = SrcManager.getExpansionLoc(BeginLoc);
  Index = LocIndex(
      util::getNormalizedPath(SrcManager.getFilename(ExpandedBeginLoc)),
      SrcManager.getExpansionLineNumber(ExpandedBeginLoc),
      SrcManager.getExpansionColumnNumber(ExpandedBeginLoc));
}

const LocIndex &ASTNode::getIndex() const { return Index; }

const DynTypedNode &ASTNode::getNode() const { return Node; }

ASTNode::nodeType ASTNode::getNodeType() const { return NodeType; }

size_t ASTNode::getOffset() const { return Offset; }

unsigned ASTNode::getLength() const { return Length; }

const QualType &ASTNode::getType() const { return Ty; }

ASTUnit &ASTNode::getASTUnit() const { return Unit; }

llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const ASTNode &Src) {
  O << Src.getIndex().getIDAsString() << ":" << Src.Offset << ":" << Src.Length;
  return O;
}

std::pair<unsigned, unsigned>
ASTNode::getLengthAndOffset(const DynTypedNode &Node,
                            const SourceLocation &BeginLoc,
                            const SourceManager &SrcManager) const {
  auto EndLoc = getEndLoc(Node, BeginLoc, SrcManager);
  auto BeginOffset = SrcManager.getDecomposedExpansionLoc(BeginLoc).second;
  auto EndOffset = SrcManager.getDecomposedExpansionLoc(EndLoc).second;
  unsigned Length = 0;
  if (EndOffset < BeginOffset)
    Length = 0xFFFFFFFF;
  else {
    Length = EndOffset - Offset +
             Lexer::MeasureTokenLength(EndLoc, SrcManager, LangOptions());
  }
  return {Length, BeginOffset};
}

SourceLocation
ASTNode::getBeginLoc(const clang::ast_type_traits::DynTypedNode &Node,
                     const clang::SourceManager &SrcManager) const {
  SourceLocation Result;
  if (const auto *AST = Node.get<Decl>())
    Result = AST->getLocation();
  else if (const auto *AST = Node.get<CXXCtorInitializer>())
    Result = AST->getSourceRange().getBegin();
  else if (const auto *AST = Node.get<CXXOperatorCallExpr>())
    Result = AST->getOperatorLoc();
  else if (const auto *AST = Node.get<CXXMemberCallExpr>())
    Result = AST->getExprLoc();
  else if (const auto *AST = Node.get<Stmt>())
    Result = AST->getBeginLoc();
  else
    assert(false && "Unimplemented");
  return SrcManager.getTopMacroCallerLoc(Result);
}

SourceLocation ASTNode::getEndLoc(const DynTypedNode &Node,
                                  const SourceLocation &BeginLoc,
                                  const SourceManager &SrcManager) const {
  auto Range = util::getMacroFunctionExpansionRange(SrcManager, BeginLoc);
  if (Range.isValid())
    return Range.getEnd();

  SourceLocation Result;
  if (const auto *AST = Node.get<Decl>())
    Result = AST->getEndLoc();
  else if (const auto *AST = Node.get<CXXCtorInitializer>())
    Result = AST->getSourceRange().getEnd();
  else if (const auto *AST = Node.get<Stmt>())
    Result = AST->getEndLoc();
  assert(Result.isValid() && "Unimplemented");

  return SrcManager.getTopMacroCallerLoc(Result);
}

QualType
ASTNode::getType(const clang::ast_type_traits::DynTypedNode &Node) const {
  if (const auto *AST = Node.get<VarDecl>())
    return AST->getType();

  if (const auto *AST = Node.get<CXXCtorInitializer>()) {
    auto *FD = AST->getAnyMember();
    if (!FD)
      throw std::runtime_error("Unknown nullptr of LLVM instance");
    return FD->getType();
  }

  if (const auto *AST = Node.get<ReturnStmt>()) {
    auto *RetValue = AST->getRetValue();
    if (!RetValue)
      return QualType();
    return RetValue->getType();
  }

  if (const auto *AST = Node.get<Expr>())
    return AST->getType();

  assert(false && "Not implemented");
}

} // namespace ftg
