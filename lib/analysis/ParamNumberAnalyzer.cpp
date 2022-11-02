#include "ParamNumberAnalyzer.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/IR/Constant.h"

namespace ftg {

ParamNumberAnalyzer::ParamNumberAnalyzer(
    const std::vector<clang::ASTUnit *> &ASTUnits, const llvm::Module &M,
    std::set<std::string> FuncNames) {
  std::map<std::string, const llvm::Constant *> GlobalAliasMap;
  for (const auto &A : M.aliases())
    GlobalAliasMap.emplace(A.getName().str(), A.getAliasee());

  for (auto *ASTUnit : ASTUnits) {
    for (auto *FD :
         util::findDefinedFunctionDecls(ASTUnit->getASTContext(), FuncNames)) {
      if (!FD)
        continue;

      // NOTE: Unexpected when multiple functions are matched by one clang
      // NamedDecl.
      const llvm::Function *F = nullptr;
      for (auto &MangledName : util::getMangledNames(*FD)) {
        auto FuncName = MangledName;
        auto Iter = GlobalAliasMap.find(MangledName);
        if (Iter != GlobalAliasMap.end() && Iter->second)
          FuncName = Iter->second->getName();
        F = M.getFunction(FuncName);
        if (F)
          break;
      }
      if (!F)
        continue;

      unsigned BeginIndex = 0;
      if (F->hasStructRetAttr())
        BeginIndex += 1;

      if (llvm::isa<clang::CXXMethodDecl>(FD) && !FD->isStatic())
        BeginIndex += 1;

      Report.add(F->getName().str(), F->arg_size(), BeginIndex);
    }
  }
}

std::unique_ptr<AnalyzerReport> ParamNumberAnalyzer::getReport() {
  return std::make_unique<ParamNumberAnalysisReport>(Report);
}

} // namespace ftg
