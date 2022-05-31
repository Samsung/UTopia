#ifndef FTG_INPUTANALYSIS_DEFMAPGENERATOR_H
#define FTG_INPUTANALYSIS_DEFMAPGENERATOR_H

#include "ftg/astirmap/ASTDefNode.h"
#include "ftg/constantanalysis/ASTValue.h"
#include "ftg/inputanalysis/ASTIRNode.h"
#include "ftg/inputanalysis/Definition.h"
#include "ftg/type/Type.h"
#include "ftg/utanalysis/UTLoader.h"
#include "ftg/utils/json/json.h"
#include "clang/AST/RecursiveASTVisitor.h"

namespace ftg {

class DefMapGenerator {
public:
  void generate(std::vector<ASTIRNode> &Nodes, const UTLoader &Loader,
                std::string BaseDir = "");
  const std::map<unsigned, std::shared_ptr<Definition>> &getDefMap() const;
  unsigned getID(ASTDefNode &ASTDef) const;

private:
  struct DeclStmtFinder : public clang::RecursiveASTVisitor<DeclStmtFinder> {
    const clang::VarDecl &D;
    const clang::DeclStmt *Result;
    DeclStmtFinder(const clang::VarDecl &D);
    bool VisitDeclStmt(const clang::DeclStmt *S);
  };
  struct DefLocKey {
    std::string Path;
    unsigned Offset;

    bool operator<(const DefLocKey &RHS) const;
  };

  std::map<DefLocKey, std::shared_ptr<Definition>> KeyDefMap;
  std::map<unsigned, std::shared_ptr<Definition>> IDDefMap;

  Definition &getDefinition(unsigned ID);
  unsigned getGlobalEndOffset(const clang::VarDecl &D,
                              const clang::SourceManager &SrcManager) const;
  unsigned getNonGlobalEndOffset(const clang::VarDecl &D,
                                 const clang::SourceManager &SrcManager) const;
  const clang::Decl *
  getFirstVarDeclhasSameBeginLoc(const clang::VarDecl &D) const;
  std::pair<unsigned, std::string>
  getDeclTypeInfo(const clang::VarDecl &D,
                  const clang::SourceManager &SrcManager) const;
  unsigned getEndOffset(const clang::VarDecl &D,
                        const clang::SourceManager &SrcManager) const;
  std::string getNamespaceName(const clang::NamedDecl &D) const;
  std::pair<std::map<unsigned, std::set<unsigned>>,
            std::map<unsigned, std::set<unsigned>>>
  generateArrayMap(std::vector<ASTIRNode> &Nodes,
                   const ArrayAnalysisReport &ArrayReport) const;
  std::tuple<unsigned, std::string, unsigned, std::string, std::string>
  generateDeclInfo(ASTDefNode &Node) const;
  Definition::DeclType generateDeclType(ASTDefNode &Node) const;
  Definition generateDefinition(ASTIRNode &Node, const UTLoader &Loader) const;
  std::vector<std::shared_ptr<Definition>>
  generateDefinitions(std::vector<ASTIRNode> &Nodes, const UTLoader &Loader,
                      const std::string &BaseDir) const;
  std::map<DefLocKey, std::shared_ptr<Definition>> generateKeyDefMap(
      std::vector<std::shared_ptr<Definition>> &Definitions) const;
  std::map<unsigned, std::shared_ptr<Definition>>
  generateIDDefMap(std::vector<std::shared_ptr<Definition>> &Definitions) const;
  std::tuple<std::string, unsigned, unsigned>
  generateLocation(const ASTDefNode &ADN) const;
  std::shared_ptr<Type> generateType(ASTDefNode &Node) const;
  ASTValue generateValue(ASTDefNode &Node,
                         const ConstAnalyzerReport &ConstReport) const;
  bool isAggregateDeclInitWOAssignOperator(const ASTDefNode &Node) const;
  bool isAssignOperatorRequired(const ASTDefNode &Node) const;
  bool isBufferAllocSize(const ASTIRNode &Node,
                         const AllocAnalysisReport &AllocReport) const;
  bool isFilePath(ASTIRNode &Node,
                  const FilePathAnalysisReport &FilePathReport) const;
  bool isLoopExit(const RDNode &Node,
                  const LoopAnalysisReport &LoopReport) const;
  bool isVarReference(const ASTDefNode &Node) const;
  Definition loadDefinition(const Json::Value &Root,
                            TargetLib &TargetReport) const;
  void updateDefinitions(
      const std::map<unsigned, std::set<unsigned>> &ArrayIDMap,
      const std::map<unsigned, std::set<unsigned>> &ArrayLenIDMap);
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_DEFMANAGER_H
