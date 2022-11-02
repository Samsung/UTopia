#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/astirmap/CalledFunctionMacroMapper.h"
#include "ftg/astirmap/IRNode.h"
#include "ftg/utils/ASTUtil.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTUnit.h"

using namespace clang;
using namespace ast_matchers;

namespace ftg {

DebugInfoMap::DebugInfoMap(const SourceCollection &SC)
    : Mapper(std::make_unique<CalledFunctionMacroMapper>()) {
  for (auto *Unit : SC.getASTUnits()) {
    if (!Unit)
      continue;

    update(*Unit);
  }
}

ASTDefNode *DebugInfoMap::getASTDefNode(const llvm::Value &V, int OIdx) {
  ASTDefNode *Result = nullptr;

  if (isa<llvm::CallBase>(&V) && OIdx >= 0) {
    const auto &CB = *llvm::dyn_cast<llvm::CallBase>(&V);
    try {
      OIdx -= getDiffNumArgs(CB);
      if (OIdx >= 0)
        return getFromArgMap(CB, OIdx);
    } catch (std::runtime_error &E) {
      return nullptr;
    }
  }

  if (const auto *I = dyn_cast<llvm::Instruction>(&V))
    Result = getFromDefMap(*I);
  else if (const auto *G = dyn_cast<llvm::GlobalValue>(&V))
    Result = getFromGVMap(*G);
  if (!Result)
    return nullptr;

  // NOTE: ignore template type
  if (isTemplateType(*Result))
    return nullptr;
  if (isNullType(*Result))
    return nullptr;
  return Result;
}

unsigned DebugInfoMap::getDiffNumArgs(const llvm::CallBase &CB) const {
  auto *ACN = getFromCallMap(CB);
  if (!ACN)
    throw std::runtime_error("CallNode not found");

  auto *E = ACN->getNode().get<Expr>();
  if (!E)
    throw std::runtime_error("CallNode has non Expr Node");

  auto *FuncDecl = util::getFunctionDecl(*(const_cast<Expr *>(E)));
  if (!FuncDecl)
    throw std::runtime_error("FunctionDecl not found");

  unsigned Result = 0;
  auto *MethodDecl = dyn_cast<clang::CXXMethodDecl>(FuncDecl);
  if (MethodDecl && !MethodDecl->isStatic())
    Result++;
  if (CB.hasStructRetAttr())
    Result++;

  return Result;
}

bool DebugInfoMap::hasDiffNumArgs(const llvm::CallBase &CB) const {
  auto *ACN = getFromCallMap(CB);
  if (!ACN)
    throw std::runtime_error("CallNode not found");

  const auto *E = ACN->getNode().get<Expr>();
  if (!E)
    throw std::runtime_error("CallNode has non Expr Node");

  unsigned ASTArgNum = 0;
  try {
    ASTArgNum = util::getArgExprs(*const_cast<Expr *>(E)).size();
  } catch (std::invalid_argument &E) {
    throw std::runtime_error("CallNode has non CallExpr Node");
  }
  auto IRArgNum = CB.getNumArgOperands() - getDiffNumArgs(CB);
  return ASTArgNum != IRArgNum;
}

bool DebugInfoMap::ArgMapKey::operator<(const struct ArgMapKey &Key) const {
  if (E != Key.E)
    return E < Key.E;
  return ArgNo < Key.ArgNo;
}

bool DebugInfoMap::GVMapKey::operator<(const struct GVMapKey &Key) const {
  if (Path != Key.Path)
    return Path < Key.Path;
  return Name < Key.Name;
}

std::unique_ptr<ASTDefNode>
DebugInfoMap::getArgumentNode(Expr &E, unsigned ArgNo, ASTUnit &U) const {
  auto ADN = std::make_unique<ASTDefNode>(E, ArgNo, U);
  assert(ADN && "Unexpected Program State");

  if (auto *CVD = getConstVarDecl(*ADN)) {
    auto *Assigned = ADN->getAssigned();
    assert(Assigned && "Unexpected Program State");

    ADN = std::make_unique<ASTDefNode>(*CVD, Assigned->getASTUnit());
    assert(ADN && "Unexpected Program State");
  }
  return ADN;
}

ASTNode *DebugInfoMap::getFromCallMap(const llvm::CallBase &CB) const {
  auto *TempCB = const_cast<llvm::CallBase *>(&CB);
  IRNode Node(*TempCB);
  if (Macros.find(Node.getIndex()) != Macros.end()) {
    if (!Mapper)
      return nullptr;
    return Mapper->getASTNode(*TempCB);
  }

  auto Iter = CallMap.find(Node.getIndex());
  if (Iter == CallMap.end())
    return nullptr;

  return Iter->second.get();
}

ASTDefNode *DebugInfoMap::getFromArgMap(const llvm::CallBase &CB,
                                        unsigned ArgNo) {
  if (hasDiffNumArgs(CB))
    return nullptr;

  auto *ACN = getFromCallMap(CB);
  if (!ACN)
    return nullptr;

  const auto *E = ACN->getNode().get<Expr>();
  if (!E)
    return nullptr;

  if (util::isImplicitArgument(*const_cast<Expr *>(E), ArgNo) ||
      util::isDefaultArgument(*const_cast<Expr *>(E), ArgNo))
    return nullptr;

  ASTDefNode *Result = nullptr;
  ArgMapKey Key = {const_cast<Expr *>(E), ArgNo};
  auto ArgMapIter = ArgMap.find(Key);
  if (ArgMapIter != ArgMap.end())
    Result = ArgMapIter->second.get();
  else
    Result = updateArgMap(*ACN, ArgNo);

  if (!Result)
    return Result;

  // NOTE: If, argument comes from a macro, (10 in below case)
  // ex) #define MACRO(a, b, c) call(a, b, c, 10);
  // The location of 10 same as the MACRO, thus we should ignore this case.
  // It requires further investigation check this location carefully.
  auto &Assignee = Result->getAssignee();
  auto *Assigned = Result->getAssigned();
  assert(Assigned && "Unexpected Program State");

  auto AssigneeIndex = Assignee.getIndex();
  auto AssignedIndex = Assigned->getIndex();
  if (AssigneeIndex.getPath() == AssignedIndex.getPath() &&
      Assignee.getOffset() == Assigned->getOffset()) {
    auto &Node = Assignee.getNode();
    auto *CCE = Node.get<CXXConstructExpr>();
    if (!CCE)
      return nullptr;
    if (CCE->getType().getAsString() != "std::string")
      return nullptr;
  }
  return Result;
}

ASTDefNode *DebugInfoMap::getFromDefMap(const llvm::Instruction &I) const {
  IRNode Node(*const_cast<llvm::Instruction *>(&I));
  auto MacroIter = Macros.find(Node.getIndex());
  if (MacroIter != Macros.end())
    return Mapper->getASTDefNode(*const_cast<llvm::Instruction *>(&I));

  auto DefMapIter = DefMap.find(Node.getIndex());
  if (DefMapIter == DefMap.end())
    return nullptr;
  return DefMapIter->second.get();
}

ASTDefNode *DebugInfoMap::getFromGVMap(const llvm::GlobalValue &G) const {
  IRNode Node(*const_cast<llvm::GlobalValue *>(&G));
  GVMapKey Key = {.Path = "", .Name = Node.getName()};
  if (G.hasInternalLinkage() || G.hasPrivateLinkage())
    Key.Path = Node.getIndex().getPath();

  auto GVMapIter = GVMap.find(Key);
  if (GVMapIter == GVMap.end())
    return nullptr;
  return GVMapIter->second.get();
}

VarDecl *DebugInfoMap::getConstVarDecl(ASTDefNode &Node) const {
  auto *Assigned = Node.getAssigned();
  if (!Assigned)
    return nullptr;

  const auto *E = Assigned->getNode().get<Expr>();
  if (!E)
    return nullptr;

  auto &Ctx = Assigned->getASTUnit().getASTContext();
  const VarDecl *Result = nullptr;
  do {
    if (E->IgnoreCasts())
      E = E->IgnoreCasts();

    const auto *DRE = dyn_cast<DeclRefExpr>(E);
    if (!DRE)
      break;

    const auto *VD = DRE->getDecl();
    if (!VD)
      break;
    if (!VD->getType().isConstant(Ctx))
      return nullptr;

    Result = dyn_cast<VarDecl>(VD);
    if (!Result)
      return nullptr;
    if (!Result->hasInit())
      return nullptr;

    E = Result->getInit();
  } while (E);

  return const_cast<VarDecl *>(Result);
}

bool DebugInfoMap::isArgsInMacro(const ASTDefNode &Node, Expr &E,
                                 unsigned ArgNo, ASTUnit &U) const {
  const auto *CurAssigned = Node.getAssigned();
  assert(CurAssigned && "Unexpected Program State");

  const auto &CurLoc = CurAssigned->getIndex();
  if (ArgNo > 0 && !util::isDefaultArgument(E, ArgNo - 1) &&
      !util::isImplicitArgument(E, ArgNo - 1)) {
    auto Prev = getArgumentNode(E, ArgNo - 1, U);
    if (Prev && Prev->getAssigned() &&
        Prev->getAssigned()->getIndex() == CurLoc)
      return true;
  }

  auto ArgSize = util::getArgExprs(E).size();
  if (ArgSize > ArgNo + 1 && !util::isDefaultArgument(E, ArgNo + 1) &&
      !util::isImplicitArgument(E, ArgNo + 1)) {
    auto Next = getArgumentNode(E, ArgNo + 1, U);
    if (Next && Next->getAssigned() &&
        Next->getAssigned()->getIndex() == CurLoc)
      return true;
  }
  return false;
}

bool DebugInfoMap::isNullType(const ASTDefNode &Node) const {
  QualType T;
  if (const auto *Assigned = Node.getAssigned())
    T = Assigned->getType();
  else
    T = Node.getAssignee().getType();
  return !T.getTypePtrOrNull();
}

bool DebugInfoMap::isTemplateType(const ASTDefNode &Node) const {
  auto *TypePtr = Node.getAssignee().getType().getTypePtrOrNull();
  if (!TypePtr)
    return false;

  auto *PointeeTypePtr = TypePtr->getPointeeType().getTypePtrOrNull();
  while (PointeeTypePtr != nullptr && TypePtr != PointeeTypePtr) {
    TypePtr = PointeeTypePtr;
    PointeeTypePtr = TypePtr->getPointeeType().getTypePtrOrNull();
  }
  return TypePtr->isTemplateTypeParmType();
}

void DebugInfoMap::update(clang::ASTUnit &Unit) {
  updateVarDecls(Unit);
  updateAssignOperators(Unit);
  updateCallExprs(Unit);
  updateCtorInitializers(Unit);
  updateReturnStmts(Unit);
}

ASTDefNode *DebugInfoMap::updateArgMap(ASTNode &ACN, unsigned ArgNo) {
  auto &U = ACN.getASTUnit();
  auto *E = const_cast<Expr *>(ACN.getNode().get<Expr>());
  assert(E && "Unexpected Program State");

  auto Cur = getArgumentNode(*E, ArgNo, U);
  assert(Cur && "Unexpected Program State");

  // NOTE: This is not to insert argument location if previous or next argument
  // have same location, which is one of the approaches to identify multiple
  // arguments are in one macro like below.
  // #define MACRO 0, 0 API(MACRO);
  // So far, our algorithm can not change a token in a macro which has more than
  // one tokens.
  if (!Cur->getAssigned() || isArgsInMacro(*Cur, *E, ArgNo, U))
    Cur.reset();

  ArgMapKey Key = {.E = E, .ArgNo = ArgNo};
  auto Result = ArgMap.emplace(Key, std::move(Cur));
  if (!Result.second)
    return nullptr;
  return Result.first->second.get();
}

void DebugInfoMap::updateCallMap(std::unique_ptr<ASTNode> Node) {
  if (!Node || !Mapper)
    return;

  const auto *E = Node->getNode().get<Expr>();
  if (!E)
    return;

  auto &Unit = Node->getASTUnit();
  if (updateMacro(util::getDebugLoc(*E), Unit)) {
    Mapper->insertMacroNode(*const_cast<Expr *>(E), Unit);
    return;
  }

  CallMap.emplace(Node->getIndex(), std::move(Node));
}

void DebugInfoMap::updateDefMap(std::unique_ptr<ASTDefNode> ADN) {
  if (!ADN)
    return;

  auto &Unit = ADN->getAssignee().getASTUnit();
  auto Loc = ADN->getSourceLocation();
  if (updateMacro(Loc, Unit))
    return;

  if (auto *CVD = getConstVarDecl(*ADN)) {
    auto *Assigned = ADN->getAssigned();
    assert(Assigned && "Unexpected Program State");
    DefMap.emplace(ADN->getLocIndex(),
                   std::make_unique<ASTDefNode>(*CVD, Assigned->getASTUnit()));
    return;
  }

  DefMap.emplace(ADN->getLocIndex(), std::move(ADN));
}

void DebugInfoMap::updateGVMap(std::unique_ptr<ASTDefNode> ADN) {
  if (!ADN)
    return;

  auto *D = ADN->getAssignee().getNode().get<VarDecl>();
  if (!D)
    return;

  auto LinkScope = D->getFormalLinkage();
  DebugInfoMap::GVMapKey Key = {.Path = "", .Name = D->getName().str()};
  if (LinkScope == NoLinkage || LinkScope == InternalLinkage) {
    Key.Path = ADN->getLocIndex().getPath();
  }
  GVMap.emplace(Key, std::move(ADN));
}

bool DebugInfoMap::updateMacro(SourceLocation Loc, clang::ASTUnit &Unit) {
  auto &SrcManager = Unit.getSourceManager();
  if (!SrcManager.isMacroBodyExpansion(Loc) &&
      !SrcManager.isMacroArgExpansion(Loc))
    return false;

  Macros.emplace(SrcManager, Loc);
  return true;
}

void DebugInfoMap::updateAssignOperators(clang::ASTUnit &Unit) {
  const std::string Tag = "Tag";
  auto &Ctx = Unit.getASTContext();
  auto Matcher = binaryOperator(unless(isExpansionInSystemHeader()),
                                isAssignmentOperator())
                     .bind(Tag);
  for (auto &Node : match(Matcher, Ctx)) {
    auto *S = Node.getNodeAs<BinaryOperator>(Tag);
    if (!S)
      continue;

    auto ADN =
        std::make_unique<ASTDefNode>(*const_cast<BinaryOperator *>(S), Unit);
    if (!ADN)
      continue;

    updateDefMap(std::move(ADN));
  }
}

void DebugInfoMap::updateCallExprs(clang::ASTUnit &Unit) {
  const std::string Tag = "Tag";
  auto &Ctx = Unit.getASTContext();

  std::vector<const Expr *> Records;
  auto Matcher =
      expr(anyOf(callExpr(unless(isExpansionInSystemHeader())),
                 cxxMemberCallExpr(unless(isExpansionInSystemHeader())),
                 ignoringElidableConstructorCall(
                     cxxConstructExpr(unless(isExpansionInSystemHeader()))),
                 cxxNewExpr(unless(isExpansionInSystemHeader())),
                 cxxDeleteExpr(unless(isExpansionInSystemHeader()))))
          .bind(Tag);
  for (auto &Node : match(Matcher, Ctx)) {
    const auto *Record = Node.getNodeAs<Expr>(Tag);
    if (!Record)
      continue;

    updateCallMap(std::move(std::make_unique<ASTNode>(
        ASTNode::CALL, DynTypedNode::create(*Record), Unit)));
    updateDefMap(
        std::make_unique<ASTDefNode>(*const_cast<Expr *>(Record), Unit));
  }
}

void DebugInfoMap::updateCtorInitializers(clang::ASTUnit &Unit) {
  const std::string Tag = "Tag";
  auto &Ctx = Unit.getASTContext();
  auto Matcher = cxxConstructorDecl(unless(isExpansionInSystemHeader()),
                                    hasAnyConstructorInitializer(anything()))
                     .bind(Tag);
  for (auto &Node : match(Matcher, Ctx)) {
    auto *Record = Node.getNodeAs<CXXConstructorDecl>(Tag);
    if (!Record)
      continue;

    for (const auto *Iter : Record->inits()) {
      assert(Iter && "Unexpected behavior of cxxConstructorDecl matcher");
      // Ignore field initializer that uses compiler generated implicit value
      if (Iter->getInit()->getStmtClass() ==
          clang::Stmt::ImplicitValueInitExprClass)
        continue;

      try {
        updateDefMap(std::move(std::make_unique<ASTDefNode>(
            *const_cast<CXXCtorInitializer *>(Iter), Unit)));
      } catch (std::exception &E) {
      }
    }
  }
}

void DebugInfoMap::updateReturnStmts(clang::ASTUnit &Unit) {
  const std::string Tag = "Tag";
  auto &Ctx = Unit.getASTContext();
  auto Matcher = returnStmt(unless(isExpansionInSystemHeader()),
                            hasReturnValue(anything()))
                     .bind(Tag);
  for (auto &Node : match(Matcher, Ctx)) {
    auto *S = Node.getNodeAs<ReturnStmt>(Tag);
    if (!S)
      continue;

    updateDefMap(std::move(
        std::make_unique<ASTDefNode>(*const_cast<ReturnStmt *>(S), Unit)));
  }
}

void DebugInfoMap::updateVarDecls(clang::ASTUnit &Unit) {
  const std::string Tag = "Tag";
  auto &Ctx = Unit.getASTContext();
  auto Matcher = varDecl(unless(anyOf(isExpansionInSystemHeader(),
                                      parmVarDecl(), isImplicit())))
                     .bind(Tag);
  for (auto &Node : match(Matcher, Ctx)) {
    auto *Record = Node.getNodeAs<VarDecl>(Tag);
    if (!Record)
      continue;

    auto ADN =
        std::make_unique<ASTDefNode>(*const_cast<VarDecl *>(Record), Unit);
    if (!ADN)
      continue;

    if (Record->hasGlobalStorage()) {
      updateGVMap(std::move(ADN));
      continue;
    }
    updateDefMap(std::move(ADN));
  }
}

} // namespace ftg
