#include "ftg/tcanalysis/TCTExtractor.h"
#include "ftg/utils/ASTUtil.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTUnit.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

TCTExtractor::TCTExtractor(const SourceCollection &SC) : TCExtractor(SC) {}

std::vector<Unittest> TCTExtractor::extractTCs(const ASTContext &Ctx) const {
  std::vector<Unittest> TCs;
  auto *TCArray = getTCArray(Ctx);
  if (!TCArray)
    return {};
  return parseTCArray(*TCArray);
}

std::vector<Unittest> TCTExtractor::extractTCs() {
  std::vector<Unittest> TCs;
  for (const auto *ASTUnit : SrcCollection.getASTUnits()) {
    if (!ASTUnit)
      continue;

    auto ExtractTCs = extractTCs(ASTUnit->getASTContext());
    TCs.insert(TCs.end(), ExtractTCs.begin(), ExtractTCs.end());
  }
  return TCs;
}

const llvm::Function *TCTExtractor::getFunc(const FunctionDecl *D) const {
  if (!D)
    return nullptr;

  auto Names = util::getMangledNames(*D);
  if (Names.size() != 1)
    return nullptr;

  const auto &M = SrcCollection.getLLVMModule();
  return M.getFunction(Names[0]);
}

const FunctionDecl *
TCTExtractor::getDefinedFuncDecl(const clang::Expr *E) const {
  const auto *DRE = dyn_cast_or_null<DeclRefExpr>(E);
  if (!DRE)
    return nullptr;

  const auto *D = dyn_cast_or_null<FunctionDecl>(DRE->getDecl());
  if (!D)
    return nullptr;

  std::string Tag = "DefinedFuncDecl";
  auto Matcher = functionDecl(hasBody(compoundStmt()), isDefinition(),
                              hasName(D->getNameAsString()))
                     .bind(Tag);

  for (const auto *ASTUnit : SrcCollection.getASTUnits())
    for (auto &Node :
         match(Matcher, *const_cast<ASTContext *>(&ASTUnit->getASTContext()))) {
      const auto *FD = Node.getNodeAs<FunctionDecl>(Tag);
      if (!FD)
        continue;
      return FD;
    }
  return nullptr;
}

const VarDecl *TCTExtractor::getTCArray(const ASTContext &Ctx) const {
  std::string Tag = "TCArray";
  auto Matcher = varDecl(hasGlobalStorage(), hasInitializer(initListExpr()),
                         hasName("tc_array"))
                     .bind(Tag);
  std::vector<const VarDecl *> VarDecls;
  for (auto &Node : match(Matcher, *const_cast<ASTContext *>(&Ctx))) {
    const auto *D = Node.getNodeAs<VarDecl>(Tag);
    if (!D)
      continue;

    VarDecls.emplace_back(D);
  }

  assert(VarDecls.size() < 2 && "Unexpected Program State");
  if (VarDecls.size() == 0)
    return nullptr;
  return VarDecls[0];
}

std::vector<Unittest> TCTExtractor::parseTCArray(const VarDecl &D) const {
  std::vector<Unittest> TCs;
  const auto *Init = D.getInit();
  if (!Init)
    return {};

  const auto *ILE = dyn_cast_or_null<InitListExpr>(Init->IgnoreCasts());
  if (!ILE)
    return {};

  for (const auto *Child : ILE->children()) {
    const auto *E = dyn_cast_or_null<Expr>(Child);
    if (!E)
      continue;

    const auto *ILE = dyn_cast_or_null<InitListExpr>(E->IgnoreCasts());
    if (!ILE)
      continue;

    auto TC = parseTCArrayElement(*ILE);
    if (!TC)
      continue;

    TCs.emplace_back(*TC);
  }

  return TCs;
}

std::unique_ptr<Unittest>
TCTExtractor::parseTCArrayElement(const InitListExpr &E) const {
  std::vector<const Expr *> Elements;
  for (const auto *Child : E.children()) {
    const auto *E = dyn_cast_or_null<Expr>(Child);
    if (!E)
      continue;

    E = E->IgnoreCasts();
    if (!E)
      continue;

    Elements.emplace_back(E);
  }

  if (Elements.size() != 4)
    return nullptr;

  const auto *BodyFD = getDefinedFuncDecl(Elements[1]);
  if (!BodyFD)
    return nullptr;

  const auto *StartupF = getFunc(getDefinedFuncDecl(Elements[2]));
  const auto *BodyF = getFunc(BodyFD);
  const auto *CleanupF = getFunc(getDefinedFuncDecl(Elements[3]));
  std::vector<FunctionNode> TCFuncs;
  if (StartupF)
    TCFuncs.emplace_back(*StartupF, false);
  TCFuncs.emplace_back(*BodyF, true);
  if (CleanupF)
    TCFuncs.emplace_back(*CleanupF, false);
  std::vector<Unittest> TCs;
  return std::make_unique<Unittest>(*BodyFD, "tct", TCFuncs);
}
