#include "ftg/astirmap/CalledFunctionMacroMapper.h"
#include "ftg/astirmap/IRNode.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"

using namespace clang;
using namespace ftg;

void CalledFunctionMacroMapper::insertMacroNode(clang::Stmt &S, ASTUnit &U) {

  if (!isa<Expr>(S))
    return;

  auto &E = *dyn_cast<Expr>(&S);
  if (!util::isCallRelatedExpr(E))
    return;

  auto *FuncDecl = util::getFunctionDecl(E);
  if (!FuncDecl)
    return;

  for (auto &MangledName : util::getMangledNames(*FuncDecl)) {
    if (MangledName.empty())
      continue;

    auto BaseLoc = util::getDebugLoc(E);
    auto Loc = LocIndex(U.getSourceManager(), BaseLoc);
    auto Key =
        DBKey(Loc.getPath(), Loc.getLine(), Loc.getColumn(), MangledName);
    if (DupKey.find(Key) != DupKey.end())
      continue;

    auto CN =
        std::make_unique<ASTNode>(ASTNode::CALL, DynTypedNode::create(E), U);
    assert(CN && "Unexpected Program State");

    auto CMInsert = CallMap.emplace(Key, std::move(CN));
    if (!CMInsert.second) {
      auto &SrcManager = U.getSourceManager();

      // if it is not macro arg expansion, then it is duplicates.
      if (SrcManager.isMacroBodyExpansion(E.getExprLoc())) {
        DupKey.insert(Key);
        continue;
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

      continue;
    }

    auto DN = std::make_unique<ASTDefNode>(E, U);
    assert(DN && "Unexpected Program State");

    auto Insert = DefMap.emplace(Key, std::move(DN));
    assert(Insert.second && "Unexpected Program State");
  }
}

ASTNode *CalledFunctionMacroMapper::getASTNode(llvm::CallBase &CB) {
  auto *CF = util::getCalledFunction(CB);
  if (!CF)
    return nullptr;

  IRNode Node(CB);
  const auto &Index = Node.getIndex();
  DBKey Key(Index.getPath(), Index.getLine(), Index.getColumn(),
            CF->getName().str());
  if (DupKey.find(Key) != DupKey.end())
    return nullptr;

  auto Acc = CallMap.find(Key);
  if (Acc == CallMap.end())
    return nullptr;
  return Acc->second.get();
}

ASTDefNode *CalledFunctionMacroMapper::getASTDefNode(llvm::Instruction &I) {
  auto *CB = dyn_cast<llvm::CallBase>(&I);
  if (!CB)
    return nullptr;

  auto *F = util::getCalledFunction(*CB);
  if (!F)
    return nullptr;

  IRNode Node(I);
  const auto &Index = Node.getIndex();
  DBKey Key(Index.getPath(), Index.getLine(), Index.getColumn(),
            F->getName().str());

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
