#ifndef FTG_UTILS_LLVMUTIL_H
#define FTG_UTILS_LLVMUTIL_H

#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/utils/FileUtil.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTTypeTraits.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Mangle.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>

namespace ftg {

#if LLVM_VERSION_MAJOR >= 12
using DynTypedNode = clang::DynTypedNode;
#else
using DynTypedNode = clang::ast_type_traits::DynTypedNode;
#endif

namespace util {

static inline llvm::Function *
getCalledFunction(const llvm::CallBase &CB,
                  IndCallSolverMgr *Solver = nullptr) {
  if (const auto *CO = CB.getCalledOperand()) {
    const auto *F =
        llvm::dyn_cast_or_null<llvm::Function>(CO->stripPointerCasts());
    if (F)
      return const_cast<llvm::Function *>(F);
  }
  if (!Solver)
    return nullptr;

  return const_cast<llvm::Function *>(Solver->getCalledFunction(CB));
}

static inline llvm::StringRef
getFileContent(const clang::FileID &ID,
               const clang::SourceManager &SrcManager) {
#if LLVM_VERSION_MAJOR >= 12
  auto MemRef = SrcManager.getBufferOrNone(ID);
  if (!MemRef)
    return llvm::StringRef();

  return MemRef->getBuffer();
#else
  const auto *FE = SrcManager.getFileEntryForID(ID);
  if (!FE)
    return llvm::StringRef();

  bool Invalidity = false;
  auto *MemBuffer = const_cast<clang::SourceManager *>(&SrcManager)
                        ->getMemoryBufferForFile(FE, &Invalidity);
  if (!MemBuffer || Invalidity)
    return llvm::StringRef();

  return MemBuffer->getBuffer();
#endif
}

static inline std::vector<std::string>
getMangledNames(const clang::NamedDecl &D) {
  // NOTE: getAllManglings method causes segmentation fault when a specific
  // instance of CXXMethodDecl is given. To bypass this bug, below logic
  // is used instead and its reasonable because getAllManglings is used
  // to get more precise result for constructors and destructors only.
  // However, from the version where the bug is fixed, it would be desriable to
  // use getAllMangling for any cases to erase unnecessary conditions.
  if (llvm::isa<clang::CXXConstructorDecl>(&D) ||
      llvm::isa<clang::CXXDestructorDecl>(&D)) {
    auto MangledNames =
        clang::ASTNameGenerator(D.getASTContext()).getAllManglings(&D);
    if (MangledNames.size() == 0)
      MangledNames.emplace_back(D.getNameAsString());
    return MangledNames;
  }

  auto MangleCtx = std::unique_ptr<clang::MangleContext>(
      D.getASTContext().createMangleContext());
  if (!MangleCtx)
    return {D.getNameAsString()};

  if (!MangleCtx->shouldMangleDeclName(&D)) {
    return {D.getNameAsString()};
  }

  std::string MangledName;
  llvm::raw_string_ostream OS(MangledName);
  MangleCtx->mangleName(&D, OS);
  OS.flush();
  return {MangledName};
}

static inline std::string getFullPath(const llvm::DIFile &F) {
  std::string Name = F.getFilename().str();
  std::string Dir = F.getDirectory().str();
  std::string Result;

  if (Name.rfind(Dir, 0) == 0)
    Result = Name;
  else if (Dir.size() > 0 && Dir.back() == '/')
    Result = Dir + Name;
  else
    Result = Dir + '/' + Name;

  return util::getNormalizedPath(Result);
}

} // namespace util
} // namespace ftg

#endif // FTG_UTILS_LLVMUTIL_H
