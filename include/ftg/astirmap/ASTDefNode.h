#ifndef FTG_ASTIRMAP_ASTDEFNODE_H
#define FTG_ASTIRMAP_ASTDEFNODE_H

#include "ftg/astirmap/ASTNode.h"
#include "ftg/astirmap/LocIndex.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/Frontend/ASTUnit.h"

namespace ftg {

class ASTDefNode {
public:
  ASTDefNode(clang::VarDecl &D, clang::ASTUnit &Unit);
  ASTDefNode(clang::BinaryOperator &B, clang::ASTUnit &Unit);
  ASTDefNode(clang::Expr &E, clang::ASTUnit &Unit);
  ASTDefNode(clang::Expr &E, unsigned ArgIdx, clang::ASTUnit &Unit);
  ASTDefNode(clang::ReturnStmt &S, clang::ASTUnit &Unit);
  ASTDefNode(clang::CXXCtorInitializer &CCI, clang::ASTUnit &U);

  LocIndex getLocIndex() const;
  const ASTNode &getAssignee() const;
  const ASTNode *getAssigned() const;
  const ASTNode *getNodeForType() const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, const ASTDefNode &);

private:
  LocIndex SourceLoc;
  std::unique_ptr<ASTNode> Assignee;
  std::unique_ptr<ASTNode> Assigned;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_ASTDEFNODE_H
