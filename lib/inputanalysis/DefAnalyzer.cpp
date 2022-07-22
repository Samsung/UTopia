#include "DefAnalyzer.h"
#include "ftg/astirmap/IRNode.h"
#include "ftg/rootdefanalysis/RDAnalyzer.h"
#include "ftg/rootdefanalysis/RDExtension.h"
#include "ftg/utils/ASTUtil.h"
#include "clang/AST/Expr.h"

namespace ftg {

DefAnalyzer::DefAnalyzer(UTLoader *Loader, std::vector<Unittest> Unittests,
                         std::set<llvm::Function *> TargetFunctions,
                         std::shared_ptr<ASTIRMap> ASTMap)
    : Loader(Loader), TargetFunctions(TargetFunctions),
      Finder(std::make_unique<TargetAPIFinder>(TargetFunctions)),
      ASTMap(ASTMap), Unittests(Unittests) {
  assert(Loader && "Unexpected Program State");
  initializeRDAnalyzer(Loader->getSourceCollection(),
                       Loader->getDirectionReport(),
                       Loader->getFilePathReport());
  analyze();
}

void DefAnalyzer::analyze() {
  if (!Loader)
    return;

  auto UTDefMap = analyzeUTDefs();
  auto Defs = collectRDNodes(UTDefMap);
  auto Nodes = collectASTIRNodes(Defs);

  DefMapGenerator DG;
  DG.generate(Nodes, *Loader, Loader->getSourceCollection().getBaseDir());
  updateAPICallsInUnittests(UTDefMap, DG);

  Report = std::make_unique<InputAnalysisReport>(DG.getDefMap(), Unittests);
  assert(Report && "Unexpected Program State");
}

void DefAnalyzer::initializeRDAnalyzer(
    const SourceCollection &SC, const DirectionAnalysisReport &DirectionReport,
    const FilePathAnalysisReport &FilePathReport) {
  RDExtension Extension;

  auto NewDirectionReport = std::make_shared<DirectionAnalysisReport>();
  assert(NewDirectionReport && "Unexpected Program State");

  for (auto Iter : DirectionReport.get()) {
    NewDirectionReport->set(Iter.first, Iter.second);
  }

  for (auto *Constructor : util::collectConstructors(SC.getASTUnits())) {
    if (!Constructor)
      continue;

    auto MN = util::getMangledName(
        const_cast<clang::CXXConstructorDecl *>(Constructor));
    auto *F = Loader->getSourceCollection().getLLVMModule().getFunction(MN);
    if (!F)
      continue;

    unsigned Idx = 0;
    if (F->hasStructRetAttr())
      Idx += 1;

    auto *A = F->getArg(Idx);
    if (!A)
      continue;

    NewDirectionReport->set(*A, Dir_Out);
  }

  Extension.setDirectionReport(NewDirectionReport);

  for (const auto &Iter : FilePathReport.get()) {
    if (!Iter.second)
      continue;

    auto DecomposedKey = FilePathReport.decomposeArgPropReportKey(Iter.first);
    Extension.addTermination(std::get<0>(DecomposedKey),
                             std::get<1>(DecomposedKey));
  }

  for (const auto *Method :
       util::collectNonStaticClassMethods(SC.getASTUnits())) {
    assert(Method && "Unexpected Program State");
    auto MN = util::getMangledName(const_cast<clang::CXXMethodDecl *>(Method));
    Extension.addNonStaticClassMethod(MN);
  }

  Analyzer = std::make_unique<RDAnalyzer>(30, &Extension);
}

std::map<Unittest *, std::vector<DefAnalyzer::FuncDef>>
DefAnalyzer::analyzeUTDefs() {
  std::map<Unittest *, std::vector<DefAnalyzer::FuncDef>> Result;
  for (unsigned S = 0, E = Unittests.size(); S < E; ++S) {
    auto &Unittest = Unittests[S];
    llvm::outs() << "[I] Analyze RootDefinition [" << Unittest.getName()
                 << "] (" << S + 1 << " / " << E << ")\n";
    Result.emplace(&Unittest, analyzeUTDef(Unittest));
  }
  return Result;
}

std::vector<DefAnalyzer::FuncDef>
DefAnalyzer::analyzeUTDef(Unittest &UnitTest) {
  assert(Finder && Analyzer && "Unexpected Program State");

  std::vector<DefAnalyzer::FuncDef> Result;

  auto Links = UnitTest.getLinks();
  if (Links.size() == 0)
    return Result;

  Analyzer->setSearchSpace(Links);
  auto FoundCalls = Finder->findAPICallers(Links);
  std::vector<llvm::CallBase *> APICalls(FoundCalls.begin(), FoundCalls.end());
  std::sort(APICalls.begin(), APICalls.end(), [](auto *Src1, auto *Src2) {
    assert(Src1 && Src2 && "Unexpected Program State");
    return IRNode(*Src1) < IRNode(*Src2);
  });

  for (unsigned S = 0, E = APICalls.size(); S < E; ++S) {
    auto *APICall = APICalls[S];
    assert(APICall && "Unexpected Program State");

    llvm::outs() << "[I] Analyze APICall (" << S + 1 << " / " << E << ")\n";
    Result.push_back(analyzeTargetCall(*APICall));
  }

  return Result;
}

DefAnalyzer::FuncDef DefAnalyzer::analyzeTargetCall(llvm::CallBase &CB) {
  DefAnalyzer::FuncDef Result(CB);
  try {
    if (ASTMap->hasDiffNumArgs(CB))
      return Result;
  } catch (std::runtime_error &E) {
    return Result;
  }
  Result.ArgDefs = analyzeTargetCallArgs(CB);
  return Result;
}

std::vector<DefAnalyzer::ArgDef>
DefAnalyzer::analyzeTargetCallArgs(llvm::CallBase &CB) {
  std::vector<DefAnalyzer::ArgDef> Result;
  for (unsigned S = ASTMap->getDiffNumArgs(CB), E = CB.getNumArgOperands();
       S < E; ++S) {
    llvm::outs() << "[I] Analyze APIArg (" << S + 1 << " / " << E << ")\n";
    Result.push_back(analyzeTargetCallArg(CB, S));
  }
  return Result;
}

DefAnalyzer::ArgDef DefAnalyzer::analyzeTargetCallArg(llvm::CallBase &CB,
                                                      unsigned ArgIdx) {
  auto FoundRootDefs =
      Analyzer->getRootDefinitions(CB.getArgOperandUse(ArgIdx));
  std::vector<RDNode> RootDefs(FoundRootDefs.begin(), FoundRootDefs.end());
  RootDefs.erase(std::remove_if(RootDefs.begin(), RootDefs.end(),
                                [this](const auto &Node) {
                                  return this->isInvalidRDNode(*ASTMap, Node);
                                }),
                 RootDefs.end());

  std::set<RDNode> Nodes(RootDefs.begin(), RootDefs.end());
  DefAnalyzer::ArgDef Result = {.Defs = Nodes};
  return Result;
}

std::set<RDNode> DefAnalyzer::collectRDNodes(
    const std::map<Unittest *, std::vector<FuncDef>> &UTDefMap) const {
  std::set<RDNode> Result;
  for (auto Iter : UTDefMap) {
    for (auto &Func : Iter.second) {
      for (auto &Arg : Func.ArgDefs) {
        for (auto &Node : Arg.Defs) {
          auto Insert = Result.insert(Node);
          if (Insert.second)
            continue;

          auto &InsertedNode = *const_cast<RDNode *>(&*Insert.first);
          InsertedNode.addFirstUses(Node.getFirstUses());
        }
      }
    }
  }
  return Result;
}

std::vector<ASTIRNode>
DefAnalyzer::collectASTIRNodes(const std::set<RDNode> &Nodes) const {
  std::vector<ASTIRNode> Result;
  for (const auto &Node : Nodes) {
    auto D = Node.getDefinition();
    if (!D.first)
      continue;

    auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
    if (!ADN)
      continue;

    Result.emplace_back(*ADN, Node);
  }
  return Result;
}

void DefAnalyzer::updateAPICallsInUnittests(
    const std::map<Unittest *, std::vector<FuncDef>> &Map,
    const DefMapGenerator &DG) const {
  for (auto Iter : Map) {
    std::vector<APICall> APICalls;
    auto *UT = Iter.first;
    for (auto &FuncDef : Iter.second) {
      std::vector<APIArgument> APIArgs;
      for (auto &ArgDef : FuncDef.ArgDefs) {
        std::set<unsigned> DefIDs;
        for (auto &Def : ArgDef.Defs) {
          auto D = Def.getDefinition();
          if (!D.first)
            continue;

          auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
          if (!ADN)
            continue;

          try {
            DefIDs.emplace(DG.getID(*ADN));
          } catch (std::invalid_argument &E) {
            assert(false && "Unexpected Program State");
          }
        }
        APIArgs.emplace_back(DefIDs);
      }
      APICalls.emplace_back(*const_cast<llvm::CallBase *>(&FuncDef.CB),
                            APIArgs);
    }
    UT->setAPICalls(APICalls);
  }
}

bool DefAnalyzer::isInitializedLocalVariable(const RDNode &Node) const {
  auto &IR = Node.getTarget().getIR();
  if (!llvm::isa<llvm::AllocaInst>(&IR))
    return false;

  auto D = Node.getDefinition();
  if (!D.first)
    return false;

  auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
  if (!ADN || !ADN->getAssigned())
    return false;

  return true;
}

bool DefAnalyzer::isInvalidRDNode(ASTIRMap &Map, const RDNode &Node) const {
  auto D = Node.getDefinition();
  if (!D.first)
    return false;

  auto *ADN = Map.getASTDefNode(*D.first, D.second);
  // At this point, local variable should not be delcared
  // with an initializer.
  if (!ADN || (!Node.isRootDefinition() && isInitializedLocalVariable(Node)))
    return true;

  auto *Assigned = ADN->getAssigned();
  if (!Assigned)
    return false;

  const auto *E = Assigned->getNode().get<clang::Expr>();
  if (!E)
    return false;

  if (util::isMacroArgUsedHashHashExpansion(*E, Assigned->getASTUnit()))
    return true;

  return false;
}

} // namespace ftg
