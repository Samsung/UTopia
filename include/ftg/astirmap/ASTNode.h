#ifndef FTG_ASTIRMAP_ASTNODE_H
#define FTG_ASTIRMAP_ASTNODE_H

#include "ftg/astirmap/LocIndex.h"
#include "ftg/utils/LLVMUtil.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/ASTUnit.h"

namespace ftg {

class ASTNode {
public:
  enum nodeType { DECL, STMT, PARAM, CALL, CTORINIT };

  ASTNode(nodeType Type, DynTypedNode Node, clang::ASTUnit &Unit);
  const LocIndex &getIndex() const;
  const DynTypedNode &getNode() const;
  nodeType getNodeType() const;
  size_t getOffset() const;
  unsigned getLength() const;
  const clang::QualType &getType() const;
  clang::ASTUnit &getASTUnit() const;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &, const ASTNode &);

private:
  DynTypedNode Node;
  LocIndex Index;
  nodeType NodeType;
  size_t Offset = 0;
  size_t Length = 0;
  clang::QualType Ty;
  clang::ASTUnit &Unit;

  clang::SourceLocation
  getBeginLoc(const DynTypedNode &Node,
              const clang::SourceManager &SrcManager) const;
  clang::SourceLocation getEndLoc(const DynTypedNode &Node,
                                  const clang::SourceLocation &BeginLoc,
                                  const clang::SourceManager &SrcManager) const;
  std::pair<unsigned, unsigned>
  getLengthAndOffset(const DynTypedNode &Node,
                     const clang::SourceLocation &BeginLoc,
                     const clang::SourceManager &SrcManager) const;
  clang::QualType getType(const DynTypedNode &Node) const;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_ASTNODE_H
