#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/ExprCXX.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Lex/ExternalPreprocessorSource.h"

using namespace clang;
using namespace ast_matchers;

namespace ftg {

namespace util {

std::vector<const clang::CXXConstructorDecl *>
collectConstructors(const std::vector<clang::ASTUnit *> Srcs) {
  const std::string Tag = "Tag";
  std::vector<const clang::CXXConstructorDecl *> Result;
  for (auto *Src : Srcs) {
    if (!Src)
      continue;

    auto &Ctx = Src->getASTContext();
    auto Matcher = cxxConstructorDecl().bind(Tag);

    for (auto &Node : match(Matcher, Ctx)) {
      auto *Record = Node.getNodeAs<CXXConstructorDecl>(Tag);
      if (!Record)
        continue;
      Result.emplace_back(Record);
    }
  }
  return Result;
}

std::vector<const clang::CXXMethodDecl *>
collectNonStaticClassMethods(const std::vector<clang::ASTUnit *> Srcs) {
  const std::string Tag = "Tag";
  std::vector<const clang::CXXMethodDecl *> Result;
  for (auto *Src : Srcs) {
    if (!Src)
      continue;

    auto &Ctx = Src->getASTContext();
    auto Matcher = cxxMethodDecl(unless(isStaticStorageClass())).bind(Tag);
    for (const auto &Node : match(Matcher, Ctx)) {
      const auto *Record = Node.getNodeAs<CXXMethodDecl>(Tag);
      if (!Record)
        continue;
      Result.emplace_back(Record);
    }
  }
  return Result;
}

std::vector<Expr *> getArgExprs(Expr &E) {
  std::vector<Expr *> Args;
  if (auto *CE = dyn_cast<CallExpr>(&E)) {
    for (size_t S = 0, E = CE->getNumArgs(); S < E; ++S)
      Args.push_back(CE->getArg(S));
  } else if (auto *CMCE = dyn_cast<CXXMemberCallExpr>(&E)) {
    for (size_t S = 0, E = CMCE->getNumArgs(); S < E; ++S)
      Args.push_back(CMCE->getArg(S));
  } else if (auto *CCE = dyn_cast<CXXConstructExpr>(&E)) {
    for (size_t S = 0, E = CCE->getNumArgs(); S < E; ++S)
      Args.push_back(CCE->getArg(S));
  } else if (auto *CNE = dyn_cast<CXXNewExpr>(&E)) {
    for (auto S = CNE->raw_arg_begin(), E = CNE->raw_arg_end(); S != E; ++S) {
      assert(S && "Unexpected Program State");
      assert(isa<Expr>(*S) && "Unexpected Program State");
      Args.push_back(dyn_cast<Expr>(*S));
    }

    if (Args.size() > 0 && isa<CXXConstructExpr>(Args.back()))
      Args.pop_back();
  } else if (auto *CDE = dyn_cast<CXXDeleteExpr>(&E)) {
    Args.push_back(CDE->getArgument());
  } else
    throw std::invalid_argument("");

  return Args;
}

clang::SourceLocation getDebugLoc(const clang::Stmt &S) {
  if (const auto *E = dyn_cast<CXXOperatorCallExpr>(&S))
    return E->getOperatorLoc();
  else if (const auto *E = dyn_cast<CXXMemberCallExpr>(&S))
    return E->getExprLoc();
  return S.getBeginLoc();
}

FunctionDecl *getFunctionDecl(Expr &E) {
  if (auto *CE = dyn_cast<CallExpr>(&E))
    return CE->getDirectCallee();

  if (auto *CMCE = dyn_cast<CXXMemberCallExpr>(&E))
    return CMCE->getDirectCallee();

  if (auto *CCE = dyn_cast<CXXConstructExpr>(&E))
    return CCE->getConstructor();

  if (auto *CNE = dyn_cast<CXXNewExpr>(&E))
    return CNE->getOperatorNew();

  if (auto *CDE = dyn_cast<CXXDeleteExpr>(&E))
    return CDE->getOperatorDelete();

  return nullptr;
}

clang::CharSourceRange
getMacroFunctionExpansionRange(const clang::SourceManager &SrcManager,
                               const SourceLocation &Loc) {
  auto FI = SrcManager.getFileID(Loc);
  if (FI.isInvalid())
    return clang::CharSourceRange();

  bool Validity = true;
  const auto &SLE = SrcManager.getSLocEntry(FI, &Validity);
  if (!Validity || !SLE.isExpansion())
    return clang::CharSourceRange();

  const auto &ExpInfo = SLE.getExpansion();
  if (!ExpInfo.isFunctionMacroExpansion())
    return clang::CharSourceRange();
  return ExpInfo.getExpansionLocRange();
}

SourceLocation getTopMacroCallerLoc(const DynTypedNode &Node,
                                    const SourceManager &SrcManager) {
  if (const auto *D = Node.get<Decl>())
    return SrcManager.getTopMacroCallerLoc(D->getBeginLoc());
  if (const auto *S = Node.get<Stmt>())
    return SrcManager.getTopMacroCallerLoc(S->getBeginLoc());
  if (const auto *C = Node.get<CXXCtorInitializer>())
    return SrcManager.getTopMacroCallerLoc(C->getSourceRange().getBegin());
  assert(false && "Not Implemented Yet");
}

bool isDefaultArgument(clang::Expr &E, unsigned AIdx) {
  auto Args = getArgExprs(E);
  if (AIdx >= Args.size())
    throw std::invalid_argument("Request Index exceeds available arguments");

  auto *Arg = Args[AIdx];
  if (!Arg)
    throw std::runtime_error("Unexpected nullptr of llvm instance");

  return isa<CXXDefaultArgExpr>(Arg);
}

bool isImplicitArgument(clang::Expr &E, unsigned AIdx) {
  auto Args = getArgExprs(E);
  // TODO:
  // Below comparison can not distinguish invalid AIdx is given.
  if (AIdx >= Args.size())
    return true;

  auto *Arg = Args[AIdx];
  if (!Arg)
    throw std::runtime_error("Unexpected nullptr of llvm instance");

  if (isa<ImplicitValueInitExpr>(Arg)) {
    // NOTE:
    // Happened when parameter is generated implicitly in IR.
    // (ex: int* p = new int())
    return true;
  }
  return false;
}

std::vector<clang::CXXMethodDecl *> findCXXMethodDeclFromCXXRecordDecls(
    const clang::CXXRecordDecl *CurCXXRecordDecl, std::string MethodName) {
  std::vector<clang::CXXMethodDecl *> FoundCXXMethodDecls;
  if (!CurCXXRecordDecl)
    return std::vector<clang::CXXMethodDecl *>();
  for (auto Method : CurCXXRecordDecl->methods()) {
    if (Method->getNameAsString().compare(MethodName) == 0) {
      FoundCXXMethodDecls.emplace_back(Method);
    }
  }
  return FoundCXXMethodDecls;
}

bool isCallRelatedExpr(clang::Expr &E) {
  return llvm::isa<clang::CallExpr>(&E) ||
         llvm::isa<clang::CXXMemberCallExpr>(&E) ||
         llvm::isa<clang::CXXConstructExpr>(&E) ||
         llvm::isa<clang::CXXNewExpr>(&E) ||
         llvm::isa<clang::CXXDeleteExpr>(&E);
}

unsigned getBeginArgIndexForIR(const clang::FunctionDecl &AST,
                               const llvm::Function *IR) {

  unsigned BeginArgIndex = 0;
  if (auto *CMD = llvm::dyn_cast<clang::CXXMethodDecl>(&AST))
    if (!CMD->isStatic())
      BeginArgIndex += 1;
  if (IR && IR->hasStructRetAttr())
    BeginArgIndex += 1;
  return BeginArgIndex;
}

bool isCStyleStruct(const clang::RecordDecl &D) {

  // 1. Can be called without any arguments.
  // 2. Has no private members.
  if (!llvm::isa<clang::CXXRecordDecl>(&D))
    return true;
  auto &CRD = *llvm::dyn_cast<clang::CXXRecordDecl>(&D);

  for (auto *Field : CRD.fields()) {
    assert(Field && "Unexpected Program State");

    if (Field->getAccess() != clang::AS_public)
      return false;
  }

  bool HasNoParamConstructor = false;
  if (CRD.getDefinition())
    HasNoParamConstructor = CRD.hasDefaultConstructor();

  for (auto *Method : CRD.methods()) {
    assert(Method && "Unexpected Program State");

    if (Method->getAccess() != clang::AS_public)
      return false;

    if (!llvm::isa<clang::CXXConstructorDecl>(Method))
      continue;
    auto &CRD = *llvm::dyn_cast<clang::CXXConstructorDecl>(Method);

    if (CRD.getNumParams() == 0)
      HasNoParamConstructor = true;
  }

  return HasNoParamConstructor;
}

bool isCStyleStruct(const clang::TagDecl &D) {

  if (!llvm::isa<clang::RecordDecl>(&D))
    return false;
  return isCStyleStruct(*llvm::dyn_cast<clang::RecordDecl>(&D));
}

bool isCStyleStruct(const clang::QualType &T) {

  if (T->isNullPtrType())
    return false;

  auto *D = T->getAsTagDecl();
  if (!D)
    return false;

  return isCStyleStruct(*D);
}

bool isMacroArgUsedHashHashExpansion(const clang::Expr &E, clang::ASTUnit &U) {

  auto &SrcManager = U.getASTContext().getSourceManager();
  auto &Preproc = U.getPreprocessor();
  auto *ExtSrc = Preproc.getExternalSource();
  if (ExtSrc)
    ExtSrc->ReadDefinedMacros();

  if (!SrcManager.isMacroArgExpansion(E.getExprLoc()))
    return false;
  auto MacroName = Preproc.getImmediateMacroName(E.getExprLoc());

  clang::MacroInfo *MI = nullptr;
  for (auto &Macro : Preproc.macros(false)) {
    auto *II = Macro.first;
    if (!II)
      continue;
    if (II->getName() != MacroName)
      continue;

    auto *EMI = Preproc.getMacroInfo(II);
    if (!EMI)
      continue;
    if (!EMI->isFunctionLike())
      continue;

    MI = EMI;
    break;
  }

  if (!MI)
    return false;

  std::set<std::string> MacroParams;
  for (auto *Param : MI->params())
    MacroParams.insert(Param->getName().str());

  auto Tokens = MI->tokens();
  auto TokenSize = Tokens.size();
  for (int I = 0; I < (int)TokenSize - 1; ++I) {
    if (Tokens[I].isNot(clang::tok::hashhash))
      continue;
    if (Tokens[I + 1].isNot(clang::tok::identifier))
      continue;

    auto *II = Tokens[I + 1].getIdentifierInfo();
    if (!II)
      continue;
    if (MacroParams.find(II->getName().str()) == MacroParams.end())
      continue;

    return true;
  }

  return false;
}

std::set<clang::FunctionDecl *>
findDefinedFunctionDecls(clang::ASTContext &Ctx, std::set<std::string> Names) {
  std::set<clang::FunctionDecl *> Result;
  std::string MatchTag = "FunctionTag";
  auto Matcher = functionDecl(isDefinition()).bind(MatchTag);
  for (auto &Node : match(Matcher, Ctx)) {
    auto *Record = const_cast<clang::FunctionDecl *>(
        Node.getNodeAs<clang::FunctionDecl>(MatchTag));
    if (!Record)
      continue;

    if (Names.size() == 0) {
      Result.insert(Record);
      continue;
    }

    for (auto &MangledName : util::getMangledNames(*Record)) {
      if (Names.find(MangledName) != Names.end()) {
        Result.insert(Record);
        break;
      }
    }
  }
  return Result;
}

} // namespace util

} // namespace ftg
