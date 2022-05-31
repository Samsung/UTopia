//===-- ConstAnalyzer.cpp - Implementation of ConstAnalyzer ---------------===//
//
// ConstantAnalyzer extracts name and value of constants in AST.
// It uses ASTMatcher to find constant declaration.
//
//===----------------------------------------------------------------------===//

#include "ftg/constantanalysis/ConstAnalyzer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/raw_ostream.h"

using namespace ftg;

std::unique_ptr<AnalyzerReport> ConstAnalyzer::getReport() {
  auto Result = std::make_unique<ConstAnalyzerReport>();
  for (auto *ASTUnit : ASTUnits) {
    auto Constants = extractConst(*ASTUnit);
    for (auto ConstPair : Constants) {
      if (Result->addConst(ConstPair.first, ConstPair.second))
        llvm::outs() << "[D] Updated Value: " << ConstPair.first << "\n";
    }
  }
  return std::move(Result);
}

std::vector<std::pair<std::string, ASTValue>>
ConstAnalyzer::extractConst(clang::ASTUnit &AST) {
  std::vector<std::pair<std::string, ASTValue>> Constants;
  const std::string Tag = "VarDecl";
  auto &Ctx = AST.getASTContext();
  auto Matcher = clang::ast_matchers::varDecl(
                     clang::ast_matchers::unless(
                         clang::ast_matchers::isExpansionInSystemHeader()))
                     .bind(Tag);
  for (auto &Node : clang::ast_matchers::match(Matcher, Ctx)) {
    auto *Record = Node.getNodeAs<clang::VarDecl>(Tag);
    if (!Record)
      continue;

    auto *Init = Record->getInit();
    if (!Init)
      continue;

    auto T = Record->getType();
    if (T.isNull() || !T.isConstant(Ctx))
      continue;

    auto Name = Record->getQualifiedNameAsString();
    if (Name.empty())
      continue;

    ASTValue Value(*Init, Ctx);
    if (!Value.isValid())
      continue;
    Constants.emplace_back(std::make_pair(Name, Value));
  }
  return Constants;
}
