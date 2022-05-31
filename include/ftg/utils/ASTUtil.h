#ifndef FTG_UTILS_ASTUTIL_H
#define FTG_UTILS_ASTUTIL_H

#include "ftg/utils/StringUtil.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Mangle.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Function.h"

namespace ftg {

namespace util {

std::vector<const clang::CXXConstructorDecl *>
collectConstructors(const std::vector<clang::ASTUnit *> Srcs);
std::vector<const clang::CXXMethodDecl *>
collectNonStaticClassMethods(const std::vector<clang::ASTUnit *> Srcs);
std::vector<clang::Expr *> getArgExprs(clang::Expr &E);
clang::SourceLocation getDebugLoc(const clang::Stmt &S);
clang::FunctionDecl *getFunctionDecl(clang::Expr &E);
clang::CharSourceRange
getMacroFunctionExpansionRange(const clang::SourceManager &SrcManager,
                               const clang::SourceLocation &Loc);
clang::SourceLocation
getTopMacroCallerLoc(const clang::ast_type_traits::DynTypedNode &Node,
                     const clang::SourceManager &SrcManager);
bool isDefaultArgument(clang::Expr &E, unsigned AIdx);
bool isImplicitArgument(clang::Expr &E, unsigned AIdx);

static inline std::string getTypeName(const clang::TagDecl &clangS) {
  return clangS.getTypedefNameForAnonDecl()
             ? clangS.getTypedefNameForAnonDecl()
                   ->getNameAsString()   // (ex) typedef struct {}A;
             : clangS.getNameAsString(); // (ex) struct A{};
}

static inline const clang::ArrayType *getAsArrayType(clang::QualType clangQTy) {
  if (auto AdjustedTy = clangQTy->getAs<clang::AdjustedType>()) {
    if (const clang::ArrayType *OriginArrType =
            AdjustedTy->getOriginalType()->getAsArrayTypeUnsafe()) {
      return OriginArrType;
    } else if (const clang::ArrayType *AdjustedArrType =
                   AdjustedTy->getAdjustedType()->getAsArrayTypeUnsafe()) {
      return AdjustedArrType;
    }
  }
  return clangQTy->getAsArrayTypeUnsafe();
}

static inline clang::QualType getPointeeTy(const clang::QualType &clangQTy) {
  if (auto ArrType = getAsArrayType(clangQTy)) {
    return ArrType->getElementType();
  }
  return clangQTy->getPointeeType();
}

static inline bool isPointerType(const clang::QualType &clangQTy) {
  return !getPointeeTy(clangQTy).isNull();
}

static inline bool isDefinedType(const clang::QualType &clangQTy) {
  return (clangQTy->isStructureType() || clangQTy->isUnionType() ||
          clangQTy->isClassType() || clangQTy->isFunctionType() ||
          clangQTy->isEnumeralType());
}

static inline bool isPrimitiveType(const clang::QualType &clangQTy) {
  return (clangQTy->isIntegerType() || clangQTy->isFloatingType() ||
          clangQTy->isBuiltinType());
}

std::string getMangledName(clang::NamedDecl *Decl);

std::vector<clang::CXXMethodDecl *> findCXXMethodDeclFromCXXRecordDecls(
    const clang::CXXRecordDecl *CurCXXRecordDecl, std::string MethodName);
bool isCallRelatedExpr(clang::Expr &E);

unsigned getBeginArgIndexForIR(const clang::FunctionDecl &AST,
                               const llvm::Function *IR);

bool isCStyleStruct(const clang::RecordDecl &D);
bool isCStyleStruct(const clang::TagDecl &D);
bool isCStyleStruct(const clang::QualType &T);

bool isMacroArgUsedHashHashExpansion(const clang::Expr &E, clang::ASTUnit &U);
std::set<clang::FunctionDecl *>
findDefinedFunctionDecls(clang::ASTContext &Ctx,
                         std::set<std::string> Names = {});

} // namespace util

} // namespace ftg

#endif // FTG_UTILS_ASTUTIL_H
