#include "ParamNumberAnalyzer.h"
#include "ftg/utils/ASTUtil.h"
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

      auto FuncName = util::getMangledName(FD);
      auto Iter = GlobalAliasMap.find(FuncName);
      if (Iter != GlobalAliasMap.end() && Iter->second)
        FuncName = Iter->second->getName();

      const auto *F = M.getFunction(FuncName);
      if (!F)
        continue;

      unsigned BeginIndex = 0;
      if (F->hasStructRetAttr())
        BeginIndex += 1;

      if (llvm::isa<clang::CXXMethodDecl>(FD) && !FD->isStatic())
        BeginIndex += 1;

      Report.add(F->getName(), F->arg_size(), BeginIndex);
    }
  }
}

std::unique_ptr<AnalyzerReport> ParamNumberAnalyzer::getReport() {
  return std::make_unique<ParamNumberAnalysisReport>(Report);
}

} // namespace ftg
