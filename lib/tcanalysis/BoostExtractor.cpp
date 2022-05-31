#include "ftg/tcanalysis/BoostExtractor.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/IRUtil.h"

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Function.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

BoostExtractor::BoostExtractor(const SourceCollection &SC) : TCExtractor(SC) {}

std::vector<Unittest> BoostExtractor::extractTCs() {
  std::vector<Unittest> Unittests;
  const auto &M = SrcCollection.getLLVMModule();
  for (const auto &ASTUnit : SrcCollection.getASTUnits()) {
    auto UTs = getTestcasesFromASTUnit(M, *ASTUnit);
    Unittests.insert(Unittests.end(), UTs.begin(), UTs.end());
  }
  return Unittests;
}

const std::vector<Unittest>
BoostExtractor::getTestcasesFromASTUnit(const llvm::Module &M,
                                        ASTUnit &U) const {
  std::vector<Unittest> UTs;
  for (const auto *TestRecord : getTestRecords(U.getASTContext())) {
    assert(TestRecord && "Unexpected Program State");

    const auto *TestBody = getTestBody(*TestRecord, U.getASTContext());
    assert(TestBody && "Unexpected Program State");

    const auto *IR = util::getIRFunction(M, TestBody);
    assert(IR && "Unexpected Program State");

    std::vector<FunctionNode> Nodes;
    if (TestBody->getName() == TestBodyName)
      Nodes.emplace_back(*IR, true);
    UTs.emplace_back(*TestRecord, "boost", Nodes);
  }

  return UTs;
}

const std::set<const CXXRecordDecl *>
BoostExtractor::getTestRecords(clang::ASTContext &Ctx) const {
  const std::string TestMatcherTag = "test";

  auto TestRecordMatcher =
      cxxRecordDecl(has(cxxMethodDecl(hasName(TestBodyName))),
                    isDerivedFrom(hasName("BOOST_AUTO_TEST_CASE_FIXTURE")))
          .bind(TestMatcherTag);

  std::set<const CXXRecordDecl *> TestRecords;
  for (auto &Node : match(TestRecordMatcher, Ctx)) {
    const auto *Record = Node.getNodeAs<CXXRecordDecl>(TestMatcherTag);
    if (!Record)
      continue;
    if (Record->getDescribedClassTemplate())
      continue;

    TestRecords.insert(Record);
  }

  return TestRecords;
}

const FunctionDecl *BoostExtractor::getTestBody(const CXXRecordDecl &D,
                                                const ASTContext &Ctx) const {
  clang::CXXMethodDecl *TD = nullptr;
  for (auto *Method : D.methods()) {
    if (!Method)
      continue;
    if (!Method->hasBody())
      continue;

    if (Method->getNameAsString() == TestBodyName) {
      assert(!TD && "Unexpected Program State");
      TD = Method;
    }
  }
  return TD;
}
