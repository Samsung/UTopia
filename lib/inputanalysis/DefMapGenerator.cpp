#include "ftg/inputanalysis/DefMapGenerator.h"
#include "ftg/inputfilter/ArrayGroupFilter.h"
#include "ftg/inputfilter/CompileConstantFilter.h"
#include "ftg/inputfilter/ConstIntArrayLenFilter.h"
#include "ftg/inputfilter/ExternalFilter.h"
#include "ftg/inputfilter/GroupFilter.h"
#include "ftg/inputfilter/InaccessibleGlobalFilter.h"
#include "ftg/inputfilter/InvalidLocationFilter.h"
#include "ftg/inputfilter/NullPointerFilter.h"
#include "ftg/inputfilter/RawStringFilter.h"
#include "ftg/inputfilter/TypeUnavailableFilter.h"
#include "ftg/inputfilter/UnsupportTypeFilter.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "clang/AST/Type.h"

using namespace clang;

namespace ftg {

void DefMapGenerator::generate(std::vector<ASTIRNode> &Nodes,
                               const UTLoader &Loader, std::string BaseDir) {
  auto Definitions = generateDefinitions(Nodes, Loader, BaseDir);
  KeyDefMap = generateKeyDefMap(Definitions);
  IDDefMap = generateIDDefMap(Definitions);

  std::map<unsigned, std::set<unsigned>> ArrayIDMap;
  std::map<unsigned, std::set<unsigned>> ArrayLenIDMap;
  std::tie(ArrayIDMap, ArrayLenIDMap) =
      generateArrayMap(Nodes, Loader.getArrayReport());
  updateDefinitions(ArrayIDMap, ArrayLenIDMap);

  std::unique_ptr<GroupFilter> Filters = std::make_unique<ArrayGroupFilter>();
  assert(Filters && "Unexpected Program State");
  Filters->start(IDDefMap);
}

const std::map<unsigned, std::shared_ptr<Definition>> &
DefMapGenerator::getDefMap() const {
  return IDDefMap;
}

unsigned DefMapGenerator::getID(ASTDefNode &Node) const {
  auto Location = generateLocation(Node);
  DefLocKey Key = {std::get<0>(Location), std::get<1>(Location)};
  auto Acc = KeyDefMap.find(Key);
  if (Acc == KeyDefMap.end())
    throw std::invalid_argument("Not registered");
  assert(Acc->second && "Unexpected Program State");

  return Acc->second->ID;
}

DefMapGenerator::DeclStmtFinder::DeclStmtFinder(const clang::VarDecl &D)
    : D(D), Result(nullptr) {}

bool DefMapGenerator::DeclStmtFinder::VisitDeclStmt(const DeclStmt *S) {
  assert(S && "Unexpected Program State");
  for (const auto *Child : S->decls()) {
    assert(Child && "Unexpected Program State");
    if (Child != &D)
      continue;
    Result = S;
    return false;
  }
  return true;
}

bool DefMapGenerator::DefLocKey::operator<(const DefLocKey &RHS) const {
  if (Path != RHS.Path)
    return Path < RHS.Path;
  return Offset < RHS.Offset;
}

bool DefMapGenerator::isAggregateDeclInitWOAssignOperator(
    const ASTDefNode &Node) const {
  const auto &Assignee = Node.getAssignee();
  if (!Node.getAssignee().getNode().get<VarDecl>())
    return false;

  auto T = Assignee.getType();
  if (T.isNull() || !T->isAggregateType())
    return false;

  const auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return false;

  auto &SrcManager =
      const_cast<ASTNode *>(&Assignee)->getASTUnit().getSourceManager();
  auto AssigneeLoc = util::getTopMacroCallerLoc(Assignee.getNode(), SrcManager);
  auto AssignedLoc = util::getTopMacroCallerLoc(
      Assigned->getNode(),
      const_cast<ASTNode *>(Assigned)->getASTUnit().getSourceManager());

  auto FileID = SrcManager.getFileID(AssigneeLoc);
  if (FileID != SrcManager.getFileID(AssignedLoc))
    return false;

  auto SOffset = Assignee.getOffset();
  auto EOffset = Assigned->getOffset();
  if (SOffset >= EOffset)
    return false;

  auto Length = EOffset - SOffset;
  auto Content = util::getFileContent(FileID, SrcManager);
  if (Content.size() < SOffset + Length)
    return false;

  auto Code = std::string(Content.substr(SOffset, Length));
  return Code.find("=") == std::string::npos;
}

bool DefMapGenerator::isAssignOperatorRequired(const ASTDefNode &Node) const {
  return !Node.getAssigned() || isAggregateDeclInitWOAssignOperator(Node);
}

bool DefMapGenerator::isBufferAllocSize(
    const ASTIRNode &Node,
    const AllocSizeAnalysisReport &AllocSizeReport) const {
  const auto *AN = Node.AST.getNodeForType();
  if (!AN)
    return false;

  const auto *T = AN->getType().getTypePtrOrNull();
  if (!T || !T->isIntegerType())
    return false;

  auto &TNode = *const_cast<RDNode *>(&Node.IR);
  if (auto *CB = llvm::dyn_cast<llvm::CallBase>(&TNode.getLocation())) {
    if (auto *F = CB->getCalledFunction()) {
      auto FuncName = F->getName().str();
      if (TNode.getIdx() >= 0 &&
          AllocSizeReport.has(FuncName, TNode.getIdx()) &&
          AllocSizeReport.get(FuncName, TNode.getIdx()))
        return true;
    }
  }

  for (auto &FirstUse : TNode.getFirstUses()) {
    auto FuncName = FirstUse.getFunctionName();
    if (AllocSizeReport.has(FuncName, FirstUse.ArgNo) &&
        AllocSizeReport.get(FuncName, FirstUse.ArgNo))
      return true;
  }

  return false;
}

bool DefMapGenerator::isLoopExit(const RDNode &Node,
                                 const LoopAnalysisReport &LoopReport) const {
  for (const auto &Use : Node.getFirstUses()) {
    auto FuncName = Use.getFunctionName();
    auto ArgNo = Use.ArgNo;
    if (!LoopReport.has(FuncName, ArgNo))
      continue;

    auto LoopResult = LoopReport.get(FuncName, ArgNo);
    if (LoopResult.LoopExit)
      return true;
  }

  return false;
}

bool DefMapGenerator::isVarReference(const ASTDefNode &Node) const {
  auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return false;

  auto *E = Assigned->getNode().get<Expr>();
  if (!E)
    return false;

  E = E->IgnoreCasts();
  if (isa<ArraySubscriptExpr>(E))
    return true;

  return false;
}

std::tuple<std::string, unsigned, unsigned>
DefMapGenerator::generateLocation(const ASTDefNode &Node) const {
  std::string Path;
  unsigned Offset, Length;
  if (const auto *Assigned = Node.getAssigned()) {
    const auto &Index = Assigned->getIndex();
    Path = Index.getPath();
    Offset = Assigned->getOffset();
    Length = Assigned->getLength();
  } else {
    const auto &Assignee = Node.getAssignee();
    const auto &Index = Assignee.getIndex();
    Path = Index.getPath();
    Offset = Assignee.getOffset() + Assignee.getLength();
    Length = 0;
  }
  return std::make_tuple(Path, Offset, Length);
}

std::shared_ptr<Type>
DefMapGenerator::generateType(ASTDefNode &Node,
                              const TypeAnalysisReport &Report) const {
  const auto *AN = Node.getNodeForType();
  if (!AN)
    return nullptr;

  const auto &T = AN->getType();
  if (T.getTypePtrOrNull() == nullptr)
    llvm::outs() << "[E] Unexpected T.getTypePtrOrNull(): " << T << "\n";

  auto &Ctx = AN->getASTUnit().getASTContext();
  if (T->isVoidPointerType()) {
    if (const auto *E = AN->getNode().get<Expr>()) {
      E = E->IgnoreCasts();
      if (isa<StringLiteral>(E))
        return Type::createCharPointerType();
    }
  }

  auto Result = Type::createType(T, Ctx, nullptr, &Report);
  if (!Result)
    return nullptr;
  return Result;
}

ASTValue
DefMapGenerator::generateValue(ASTDefNode &Node,
                               const ConstAnalyzerReport &ConstReport) const {
  auto *ValueNode = Node.getAssigned();
  if (!ValueNode)
    return ASTValue();

  auto *ValueExpr = const_cast<Expr *>(ValueNode->getNode().get<Expr>());
  if (!ValueExpr)
    return ASTValue();

  ASTValue Result(*ValueExpr, ValueNode->getASTUnit().getASTContext());
  if (Result.isValid())
    return Result;

  ValueExpr = ValueExpr->IgnoreCasts();
  if (!ValueExpr || !isa<DeclRefExpr>(ValueExpr))
    return Result;

  auto &DRE = *dyn_cast<DeclRefExpr>(ValueExpr);
  auto *D = DRE.getDecl();
  if (!D)
    return Result;

  const auto *CV = ConstReport.getConstValue(D->getQualifiedNameAsString());
  if (!CV)
    return Result;
  return *CV;
}

Definition &DefMapGenerator::getDefinition(unsigned ID) {
  auto Iter = IDDefMap.find(ID);
  assert(Iter != IDDefMap.end() && "Unexpected Program State");

  auto &Def = Iter->second;
  assert(Def && "Unexpected Program State");

  return *Def;
}

unsigned
DefMapGenerator::getGlobalEndOffset(const VarDecl &D,
                                    const SourceManager &SrcManager) const {
  auto Loc = SrcManager.getTopMacroCallerLoc(D.getBeginLoc());
  auto FileID = SrcManager.getFileID(Loc);
  auto EndLoc = SrcManager.getLocForEndOfFile(FileID);
  if (EndLoc.isInvalid()) {
    Loc = SrcManager.getSpellingLoc(D.getBeginLoc());
    FileID = SrcManager.getFileID(Loc);
    EndLoc = SrcManager.getLocForEndOfFile(FileID);
    assert(EndLoc.isValid() && "Unexpected Program State");
  }
  return SrcManager.getFileOffset(EndLoc);
}

unsigned
DefMapGenerator::getNonGlobalEndOffset(const VarDecl &D,
                                       const SourceManager &SrcManager) const {
  const auto *DC = D.getDeclContext();
  assert(DC && "Unexpected Program State");

  DeclStmtFinder Finder(D);
  Finder.TraverseDecl(Decl::castFromDeclContext(DC));
  assert(Finder.Result && "Unexpected Program State");

  auto EndLoc = SrcManager.getTopMacroCallerLoc(Finder.Result->getEndLoc());
  auto Offset = SrcManager.getFileOffset(EndLoc);
  return Offset +
         Lexer::MeasureTokenLength(EndLoc, SrcManager, clang::LangOptions());
}

const Decl *
DefMapGenerator::getFirstVarDeclhasSameBeginLoc(const VarDecl &D) const {
  const auto *DC = D.getDeclContext();
  if (!DC)
    return &D;

  DeclStmtFinder Finder(D);
  Finder.TraverseDecl(Decl::castFromDeclContext(DC));
  if (Finder.Result)
    return *Finder.Result->decl_begin();

  for (const auto *GDecl : DC->decls()) {
    if (isa<VarDecl>(GDecl) && GDecl->getBeginLoc() == D.getBeginLoc())
      return GDecl;
  }
  return &D;
}

std::pair<unsigned, std::string>
DefMapGenerator::getDeclTypeInfo(const VarDecl &D,
                                 const SourceManager &SrcManager) const {
  auto MacroCallerLoc = SrcManager.getTopMacroCallerLoc(D.getBeginLoc());
  auto *Base = getFirstVarDeclhasSameBeginLoc(D);
  assert(Base && "Unexpected Program State");

  auto Offset = SrcManager.getFileOffset(MacroCallerLoc);
  int Length = SrcManager.getFileOffset(
                   SrcManager.getTopMacroCallerLoc(Base->getLocation())) -
               (int)Offset;
  if (Length <= 0)
    return std::make_pair(0, "");

  auto Content =
      util::getFileContent(SrcManager.getFileID(MacroCallerLoc), SrcManager);
  if (Content.size() < Offset + Length)
    return std::make_pair(0, "");

  return std::make_pair(Offset, Content.substr(Offset, Length).str());
}

unsigned DefMapGenerator::getEndOffset(const VarDecl &D,
                                       const SourceManager &SrcManager) const {
  if (!D.isLocalVarDeclOrParm())
    return getGlobalEndOffset(D, SrcManager);
  return getNonGlobalEndOffset(D, SrcManager);
}

std::string DefMapGenerator::getNamespaceName(const NamedDecl &D) const {
  const std::string AnonymousNS = "(anonymous namespace)::";
  auto Name = D.getQualifiedNameAsString();
  auto Splited = StringRef(Name).rsplit("::");
  if (Splited.second.empty())
    return "";

  auto Result = Splited.first.str() + "::";
  if (Result.find(AnonymousNS) == 0)
    Result = Result.substr(AnonymousNS.size() - 2);
  util::replaceStrAll(Result, AnonymousNS, "");
  return Result;
}

std::pair<std::map<unsigned, std::set<unsigned>>,
          std::map<unsigned, std::set<unsigned>>>
DefMapGenerator::generateArrayMap(
    std::vector<ASTIRNode> &Nodes,
    const ArrayAnalysisReport &ArrayReport) const {
  std::map<RDArgIndex, std::set<unsigned>> ArrayNodes;
  std::map<RDArgIndex, std::set<unsigned>> ArrayLenNodes;

  struct Index {
    std::string FuncName;
    unsigned ParamNo;
    bool operator<(const Index &R) const {
      if (FuncName < R.FuncName)
        return true;
      return ParamNo < R.ParamNo;
    }
  };

  std::map<Index, unsigned> ArrayLenMap;
  for (const auto &Iter : ArrayReport.get()) {
    if (Iter.second <= 0)
      continue;
    auto DecomposedKey = ArrayReport.decomposeArgPropReportKey(Iter.first);
    Index Key = {std::get<0>(DecomposedKey), (unsigned)Iter.second};
    ArrayLenMap.emplace(Key, std::get<1>(DecomposedKey));
  }

  for (auto &Node : Nodes) {
    const auto *AN = Node.AST.getNodeForType();
    assert(AN && "Unexpected Program State");

    const auto &T = AN->getType();
    for (auto &FirstUse : Node.IR.getFirstUses()) {
      auto Name = FirstUse.getFunctionName();
      if (Name.empty())
        continue;

      unsigned ID = 0;
      try {
        ID = getID(Node.AST);
      } catch (std::invalid_argument &E) {
        assert(false && "Unexpected Program State");
      }

      if (ArrayReport.has(Name, FirstUse.ArgNo) &&
          ArrayReport.get(Name, FirstUse.ArgNo) !=
              ArrayAnalysisReport::NO_ARRAY) {
        if (!util::isPointerType(T))
          continue;

        std::set<unsigned> IDs = {ID};
        auto Insert = ArrayNodes.emplace(FirstUse, IDs);
        if (!Insert.second)
          Insert.first->second.insert(ID);
        continue;
      }

      Index Key = {Name, FirstUse.ArgNo};
      auto Iter = ArrayLenMap.find(Key);
      if (Iter != ArrayLenMap.end()) {
        assert(!T.isNull() && "Unexpected Program State");
        if (!T->isIntegerType())
          continue;

        std::set<unsigned> IDs = {ID};
        auto Insert = ArrayLenNodes.emplace(FirstUse, IDs);
        if (!Insert.second)
          Insert.first->second.insert(ID);
      }
    }
  }

  std::map<unsigned, std::set<unsigned>> ArrayIDMap;
  for (auto ArrayNodeIter : ArrayNodes) {
    auto *CB = ArrayNodeIter.first.CB;
    assert(CB && "Unexpected Program State");

    auto ArgArrayNo = ArrayNodeIter.first.ArgNo;
    auto FuncName = ArrayNodeIter.first.getFunctionName();
    assert(ArrayReport.has(FuncName, ArgArrayNo) && "Unexpected Program State");
    auto ArgArrayLenNo = ArrayReport.get(FuncName, ArgArrayNo);

    std::set<unsigned> ArrayLenIDs;
    if (ArgArrayLenNo >= 0) {
      RDArgIndex Index(*CB, ArgArrayLenNo);
      auto ArrayLenNodesIter = ArrayLenNodes.find(Index);
      if (ArrayLenNodesIter != ArrayLenNodes.end()) {
        ArrayLenIDs = ArrayLenNodesIter->second;
      }
    }

    for (auto ArrayID : ArrayNodeIter.second) {
      auto Insert = ArrayIDMap.emplace(ArrayID, ArrayLenIDs);
      if (Insert.second)
        continue;

      Insert.first->second.insert(ArrayLenIDs.begin(), ArrayLenIDs.end());
    }
  }

  std::map<unsigned, std::set<unsigned>> ArrayLenIDMap;
  for (auto ArrayLenNodeIter : ArrayLenNodes) {
    auto *CB = ArrayLenNodeIter.first.CB;
    assert(CB && "Unexpected Program State");

    auto ArgArrayLenNo = ArrayLenNodeIter.first.ArgNo;
    Index Key = {ArrayLenNodeIter.first.getFunctionName(), ArgArrayLenNo};
    auto Iter = ArrayLenMap.find(Key);
    assert(Iter != ArrayLenMap.end() && "Unexpected Program State");
    auto ArgArrayNo = Iter->second;

    std::set<unsigned> ArrayIDs;
    RDArgIndex Index(*CB, ArgArrayNo);
    auto ArrayNodesIter = ArrayNodes.find(Index);
    if (ArrayNodesIter != ArrayNodes.end())
      ArrayIDs = ArrayNodesIter->second;

    for (auto ArrayLenID : ArrayLenNodeIter.second) {
      auto Insert = ArrayLenIDMap.emplace(ArrayLenID, ArrayIDs);
      if (Insert.second)
        continue;

      Insert.first->second.insert(ArrayIDs.begin(), ArrayIDs.end());
    }
  }

  return std::make_pair(ArrayIDMap, ArrayLenIDMap);
}

std::tuple<unsigned, std::string, unsigned, std::string, std::string>
DefMapGenerator::generateDeclInfo(ASTDefNode &Node) const {
  auto *D = Node.getAssignee().getNode().get<VarDecl>();
  if (!D)
    return std::make_tuple(0, "", 0, "", "");

  auto NamespaceName = getNamespaceName(*D);
  auto VariableName = dyn_cast<NamedDecl>(D)->getName();
  if (isa<DecompositionDecl>(D) && VariableName.empty())
    return std::make_tuple(0, "", 0, "", "");

  assert(!VariableName.empty() && "Unexpected Program State");

  const auto &SrcManager = Node.getAssignee().getASTUnit().getSourceManager();
  auto Offset = getEndOffset(*D, SrcManager);
  auto DeclTypeInfo = getDeclTypeInfo(*D, SrcManager);
  return std::make_tuple(DeclTypeInfo.first, DeclTypeInfo.second, Offset,
                         VariableName.str(), NamespaceName);
}

Definition::DeclType DefMapGenerator::generateDeclType(ASTDefNode &Node) const {
  const auto *D = Node.getAssignee().getNode().get<VarDecl>();
  if (!D)
    return Definition::DeclType_None;

  if (D->isStaticLocal())
    return Definition::DeclType_StaticLocal;
  if (!D->isLocalVarDeclOrParm())
    return Definition::DeclType_Global;
  return Definition::DeclType_Decl;
}

Definition DefMapGenerator::generateDefinition(ASTIRNode &Node,
                                               const UTLoader &Loader) const {
  Definition Result;
  Result.AssignOperatorRequired = isAssignOperatorRequired(Node.AST);
  Result.BufferAllocSize = isBufferAllocSize(Node, Loader.getAllocSizeReport());
  Result.FilePath = isFilePath(Node, Loader.getFilePathReport());
  Result.LoopExit = isLoopExit(Node.IR, Loader.getLoopReport());
  std::tie(Result.TypeOffset, Result.TypeString, Result.EndOffset,
           Result.VarName, Result.Namespace) = generateDeclInfo(Node.AST);
  Result.Declaration = generateDeclType(Node.AST);
  std::tie(Result.Path, Result.Offset, Result.Length) =
      generateLocation(Node.AST);
  Result.DataType = generateType(Node.AST, Loader.getTypeReport());
  Result.Value = generateValue(Node.AST, Loader.getConstReport());
  return Result;
}

std::vector<std::shared_ptr<Definition>>
DefMapGenerator::generateDefinitions(std::vector<ASTIRNode> &Nodes,
                                     const UTLoader &Loader,
                                     const std::string &BaseDir) const {
  std::unique_ptr<InputFilter> Filter = std::make_unique<NullPointerFilter>(
      std::make_unique<RawStringFilter>(std::make_unique<ExternalFilter>(
          Loader.getDirectionReport(),
          std::make_unique<TypeUnavailableFilter>(
              std::make_unique<CompileConstantFilter>(
                  std::make_unique<ConstIntArrayLenFilter>(
                      std::make_unique<InaccessibleGlobalFilter>(
                          std::make_unique<InvalidLocationFilter>(
                              BaseDir,
                              std::make_unique<UnsupportTypeFilter>()))))))));
  assert(Filter && "Unexpected Program State");

  std::map<DefLocKey, Definition> DefLocMap;
  for (auto &Node : Nodes) {
    auto Def = generateDefinition(Node, Loader);
    Filter->start(Node, Def.Filters);
    DefLocMap.emplace(DefLocKey({Def.Path, Def.Offset}), Def);
  }

  std::vector<std::shared_ptr<Definition>> Result;
  unsigned IDCounter = 0;
  for (auto &Iter : DefLocMap) {
    auto &Def = Iter.second;
    Def.ID = IDCounter++;
    Result.push_back(std::make_shared<Definition>(Def));
  }

  return Result;
}

std::map<DefMapGenerator::DefLocKey, std::shared_ptr<Definition>>
DefMapGenerator::generateKeyDefMap(
    std::vector<std::shared_ptr<Definition>> &Definitions) const {
  std::map<DefMapGenerator::DefLocKey, std::shared_ptr<Definition>> Result;

  for (auto &Definition : Definitions) {
    assert(Definition && "Unexpected Program State");
    DefLocKey Key = {Definition->Path, Definition->Offset};
    if (!Result.emplace(Key, Definition).second) {
      assert(false && "Unexpected Program State");
    }
  }

  return Result;
}

std::map<unsigned, std::shared_ptr<Definition>>
DefMapGenerator::generateIDDefMap(
    std::vector<std::shared_ptr<Definition>> &Definitions) const {
  std::map<unsigned, std::shared_ptr<Definition>> Result;

  for (auto &Definition : Definitions) {
    assert(Definition && "Unexpected Program State");
    if (!Result.emplace(Definition->ID, Definition).second) {
      assert(false && "Unexpected Program State");
    }
  }

  return Result;
}

bool DefMapGenerator::isFilePath(
    ASTIRNode &Node, const FilePathAnalysisReport &FilePathReport) const {
  auto Def = Node.IR.getDefinition();
  auto *CB = dyn_cast_or_null<llvm::CallBase>(Def.first);
  if (!CB || Def.second == -1)
    return false;

  auto *F = util::getCalledFunction(*CB);
  if (!F)
    return false;

  auto FuncName = F->getName().str();
  return FilePathReport.has(FuncName, Def.second) &&
         FilePathReport.get(FuncName, Def.second);
}

void DefMapGenerator::updateDefinitions(
    const std::map<unsigned, std::set<unsigned>> &ArrayIDMap,
    const std::map<unsigned, std::set<unsigned>> &ArrayLenIDMap) {
  for (auto ArrayIDMapIter : ArrayIDMap) {
    auto &D = getDefinition(ArrayIDMapIter.first);
    D.Array = true;
    D.ArrayLenIDs = ArrayIDMapIter.second;
  }
  for (auto ArrayLenIDMapIter : ArrayLenIDMap) {
    auto &D = getDefinition(ArrayLenIDMapIter.first);
    D.ArrayLen = true;
    D.ArrayIDs = ArrayLenIDMapIter.second;
  }
}

} // namespace ftg
