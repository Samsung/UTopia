#include "ftg/analysis/TypeAnalyzer.h"
#include "ftg/utils/ASTUtil.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace ftg;
using namespace clang;
using namespace clang::ast_matchers;

TypeAnalyzer::TypeAnalyzer(const std::vector<ASTUnit *> &Units) {
  for (const auto *Unit : Units) {
    if (!Unit)
      continue;

    collectEnums(*Unit);
  }
}

std::unique_ptr<AnalyzerReport> TypeAnalyzer::getReport() {
  return std::make_unique<TypeAnalysisReport>(&Report);
}

void TypeAnalyzer::collectEnums(const ASTUnit &Unit) {
  const std::string Tag = "Tag";
  for (const auto &Node :
       match(enumDecl().bind(Tag),
             *const_cast<ASTContext *>(&Unit.getASTContext()))) {
    auto *Record = Node.getNodeAs<EnumDecl>(Tag);
    if (!Record)
      continue;

    Report.addEnum(*Record);
  }
}
