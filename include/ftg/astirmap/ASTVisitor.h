#ifndef FTG_ASTIRMAP_ASTVISITOR_H
#define FTG_ASTIRMAP_ASTVISITOR_H

#include "ftg/astirmap/ASTNodeManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTUnit.h"

namespace ftg {

class ASTVisitor : public clang::RecursiveASTVisitor<ASTVisitor> {
public:
  ASTVisitor(ASTNodeManager &Manager, clang::ASTUnit &Unit);

  void traverse();
  bool VisitStmt(clang::Stmt *S);
  bool VisitDecl(clang::Decl *D);
  bool VisitVarDecl(clang::VarDecl *D);

private:
  ASTNodeManager &Manager;
  clang::ASTUnit *Unit;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_ASTVISITOR_H
