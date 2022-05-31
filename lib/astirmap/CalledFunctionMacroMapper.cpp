#include "ftg/astirmap/CalledFunctionMacroMapper.h"
#include "ftg/astirmap/IRNode.h"
#include "ftg/utils/ASTUtil.h"

using namespace clang;
using namespace ast_type_traits;

using namespace ftg;

void CalledFunctionMacroMapper::insertMacroNode(clang::Stmt &S, ASTUnit &U) {

  if (!isa<Expr>(S))
    return;

  auto &E = *dyn_cast<Expr>(&S);
  if (!util::isCallRelatedExpr(E))
    return;

  auto CN =
      std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), U);
  assert(CN && "Unexpected Program State");

  auto *FuncDecl = util::getFunctionDecl(E);
  if (!FuncDecl)
    return;
  auto FuncName = util::getMangledName(FuncDecl);
  if (FuncName.empty())
    return;

  auto BaseLoc = util::getDebugLoc(E);
  auto Loc = LocIndex(U.getSourceManager(), BaseLoc);
  auto Key = DBKey(Loc.getPath(), Loc.getLine(), Loc.getColumn(), FuncName);

  if (DupKey.find(Key) != DupKey.end()) {
    return;
  }

  auto CMInsert = CallMap.emplace(Key, std::move(CN));
  if (!CMInsert.second) {
    auto &SrcManager = U.getSourceManager();

    // if it is not macro arg expansion, then it is duplicates.
    if (SrcManager.isMacroBodyExpansion(E.getExprLoc())) {
      DupKey.insert(Key);
      return;
    }

    // NOTE: It is not duplicated key if it has same macro caller location if
    //       it is macro arg expansion
    auto &PrevCN = CMInsert.first->second;
    auto NewCN =
        std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), U);
    assert((PrevCN && NewCN) && "Unexpected Program State");

    auto PrevLoc = util::getTopMacroCallerLoc(
        PrevCN->getNode(), PrevCN->getASTUnit().getSourceManager());
    auto NewLoc = util::getTopMacroCallerLoc(
        NewCN->getNode(), NewCN->getASTUnit().getSourceManager());

    auto PrevLI = LocIndex(PrevCN->getASTUnit().getSourceManager(), PrevLoc);
    auto NewLI = LocIndex(NewCN->getASTUnit().getSourceManager(), NewLoc);

    if (PrevLI != NewLI)
      DupKey.insert(Key);

    return;
  }

  auto DN = std::make_unique<ASTDefNode>(E, U);
  assert(DN && "Unexpected Program State");

  auto Insert = DefMap.emplace(Key, std::move(DN));
  assert(Insert.second && "Unexpected Program State");
}

ASTNode *CalledFunctionMacroMapper::getASTNode(llvm::CallBase &CB) {

  auto *CV = CB.getCalledValue();
  if (!CV)
    return nullptr;

  CV = CV->stripPointerCasts();
  if (!CV)
    return nullptr;

  auto *CF = llvm::dyn_cast_or_null<llvm::Function>(CV);
  if (!CF)
    return nullptr;

  IRNode Node(CB);
  const auto &Index = Node.getIndex();
  DBKey Key(Index.getPath(), Index.getLine(), Index.getColumn(), CF->getName());

  if (DupKey.find(Key) != DupKey.end())
    return nullptr;

  auto Acc = CallMap.find(Key);
  if (Acc == CallMap.end())
    return nullptr;
  return Acc->second.get();
}

ASTDefNode *CalledFunctionMacroMapper::getASTDefNode(llvm::Instruction &I) {

  auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(&I);
  if (!CB)
    return nullptr;

  auto *CV = CB->getCalledValue();
  if (!CV)
    return nullptr;

  auto *F = llvm::dyn_cast_or_null<llvm::Function>(CV->stripPointerCasts());
  if (!F)
    return nullptr;

  IRNode Node(*CB);
  const auto &Index = Node.getIndex();
  DBKey Key(Index.getPath(), Index.getLine(), Index.getColumn(), F->getName());

  if (DupKey.find(Key) != DupKey.end())
    return nullptr;

  auto Acc = DefMap.find(Key);
  if (Acc == DefMap.end())
    return nullptr;
  return Acc->second.get();
}

CalledFunctionMacroMapper::DBKey::DBKey(std::string FilePath, unsigned Line,
                                        unsigned Column,
                                        std::string FunctionName)
    : FilePath(FilePath), Line(Line), Column(Column),
      FunctionName(FunctionName) {}

bool CalledFunctionMacroMapper::DBKey::operator<(const DBKey &RHS) const {
  if (FilePath != RHS.FilePath)
    return FilePath < RHS.FilePath;
  if (Line != RHS.Line)
    return Line < RHS.Line;
  if (Column != RHS.Column)
    return Line < RHS.Column;
  return FunctionName < RHS.FunctionName;
}
