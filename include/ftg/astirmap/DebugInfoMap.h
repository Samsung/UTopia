#ifndef FTG_ASTIRMAP_DEBUGINFOMAP_H
#define FTG_ASTIRMAP_DEBUGINFOMAP_H

#include "ftg/astirmap/ASTIRMap.h"
#include "ftg/astirmap/MacroMapper.h"
#include "ftg/sourceloader/SourceCollection.h"

namespace ftg {

class DebugInfoMap : public ASTIRMap {

public:
  DebugInfoMap(const SourceCollection &SC);
  ASTDefNode *getASTDefNode(const llvm::Value &V, int OIdx = -1) override;
  unsigned getDiffNumArgs(const llvm::CallBase &CB) const override;
  bool hasDiffNumArgs(const llvm::CallBase &CB) const override;

private:
  struct ArgMapKey {
    clang::Expr *E;
    unsigned ArgNo;

    bool operator<(const struct ArgMapKey &Key) const;
  };
  struct GVMapKey {
    std::string Path;
    std::string Name;
    bool operator<(const struct GVMapKey &Key) const;
  };
  std::map<ArgMapKey, std::unique_ptr<ASTDefNode>> ArgMap;
  std::map<LocIndex, std::unique_ptr<ASTNode>> CallMap;
  std::map<LocIndex, std::unique_ptr<ASTDefNode>> DefMap;
  std::map<GVMapKey, std::unique_ptr<ASTDefNode>> GVMap;
  std::unique_ptr<MacroMapper> Mapper;
  std::set<LocIndex> Macros;

  std::unique_ptr<ASTDefNode> getArgumentNode(clang::Expr &E, unsigned ArgNo,
                                              clang::ASTUnit &U) const;
  ASTNode *getFromCallMap(const llvm::CallBase &CB) const;
  ASTDefNode *getFromArgMap(const llvm::CallBase &CB, unsigned ArgNo);
  ASTDefNode *getFromDefMap(const llvm::Instruction &I) const;
  ASTDefNode *getFromGVMap(const llvm::GlobalValue &G) const;
  clang::VarDecl *getConstVarDecl(ASTDefNode &Node) const;
  bool isArgsInMacro(const ASTDefNode &Node, clang::Expr &E, unsigned ArgNo,
                     clang::ASTUnit &U) const;
  bool isNullType(const ASTDefNode &Node) const;
  bool isTemplateType(const ASTDefNode &Node) const;
  void update(clang::ASTUnit &Unit);
  ASTDefNode *updateArgMap(ASTNode &ACN, unsigned ArgNo);
  void updateCallMap(std::unique_ptr<ASTNode> ACN);
  void updateDefMap(std::unique_ptr<ASTDefNode> ADN);
  void updateGVMap(std::unique_ptr<ASTDefNode> ADN);
  bool updateMacro(clang::SourceLocation Loc, clang::ASTUnit &Unit);
  void updateAssignOperators(clang::ASTUnit &Unit);
  void updateCallExprs(clang::ASTUnit &Unit);
  void updateCtorInitializers(clang::ASTUnit &Unit);
  void updateReturnStmts(clang::ASTUnit &Unit);
  void updateVarDecls(clang::ASTUnit &Unit);
};

} // namespace ftg

#endif // FTG_ASTIRMAP_ASTINFOMAP_H
