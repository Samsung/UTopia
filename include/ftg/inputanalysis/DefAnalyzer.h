#ifndef FTG_INPUTANALYSIS_DEFANALYZER_H
#define FTG_INPUTANALYSIS_DEFANALYZER_H

#include "ftg/astirmap/ASTIRMap.h"
#include "ftg/inputanalysis/ASTIRNode.h"
#include "ftg/inputanalysis/DefMapGenerator.h"
#include "ftg/inputanalysis/InputAnalyzer.h"
#include "ftg/rootdefanalysis/RootDefAnalyzer.h"
#include "ftg/utanalysis/TargetAPIFinder.h"
#include "ftg/utanalysis/UTLoader.h"

namespace ftg {

class DefAnalyzer : public InputAnalyzer {

public:
  DefAnalyzer(UTLoader *Loader, std::vector<Unittest> Unittests,
              std::set<llvm::Function *> TargetFunctions,
              std::shared_ptr<ASTIRMap> ASTMap);

protected:
  void analyze() override;

private:
  struct ArgDef {
    std::set<RDNode> Defs;
  };

  struct FuncDef {
    const llvm::CallBase &CB;
    std::vector<ArgDef> ArgDefs;

    FuncDef(const llvm::CallBase &CB) : CB(CB) {}
  };

  UTLoader *Loader;
  std::set<llvm::Function *> TargetFunctions;
  std::unique_ptr<RootDefAnalyzer> Analyzer;
  std::unique_ptr<TargetAPIFinder> Finder;
  std::shared_ptr<ASTIRMap> ASTMap;
  std::vector<Unittest> Unittests;

  void initializeRDAnalyzer(const SourceCollection &SC,
                            const DirectionAnalysisReport &DirectionReport,
                            const FilePathAnalysisReport &FilePathReport);
  std::map<Unittest *, std::vector<FuncDef>> analyzeUTDefs();
  std::vector<FuncDef> analyzeUTDef(Unittest &UT);
  FuncDef analyzeTargetCall(llvm::CallBase &CB);
  std::vector<ArgDef> analyzeTargetCallArgs(llvm::CallBase &CB);
  ArgDef analyzeTargetCallArg(llvm::CallBase &CB, unsigned ArgIdx);

  std::set<RDNode> collectRDNodes(
      const std::map<Unittest *, std::vector<FuncDef>> &UTDefMap) const;
  std::vector<ASTIRNode> collectASTIRNodes(const std::set<RDNode> &Nodes) const;

  void updateAPICallsInUnittests(
      const std::map<Unittest *, std::vector<FuncDef>> &Map,
      const DefMapGenerator &DG) const;
  bool isInitializedLocalVariable(const RDNode &Node) const;
  bool isInvalidRDNode(ASTIRMap &Map, const RDNode &Node) const;
};

} // namespace ftg

#endif // FTG_INPUTANALYSIS_DEFANALYZER_H
