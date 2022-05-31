#include "ftg/tcanalysis/GoogleTestExtractor.h"
#include "ftg/tcanalysis/NamedDeclCallback.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/IRUtil.h"

#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Casting.h"

using namespace ftg;
using namespace clang;
using namespace ast_matchers;

GoogleTestExtractor::GoogleTestExtractor(const SourceCollection &SC)
    : TCExtractor(SC) {}

bool GoogleTestExtractor::isDescendantOf(
    const CXXRecordDecl *ClassCXXRecordDecl, std::string AncestorName) {
  if (!ClassCXXRecordDecl)
    return false;
  if (ClassCXXRecordDecl->getNameAsString().compare(AncestorName) == 0)
    return true;

  for (auto SuperClassBaseSpecifier : ClassCXXRecordDecl->bases()) {
    const auto *SuperClassBaseType =
        SuperClassBaseSpecifier.getType().getTypePtrOrNull();
    assert(SuperClassBaseType && "Unexpected Program State");
    const auto *DesugaredType =
        SuperClassBaseType->getUnqualifiedDesugaredType();

    while (SuperClassBaseType != DesugaredType) {
      SuperClassBaseType = DesugaredType;
      DesugaredType = DesugaredType->getUnqualifiedDesugaredType();
    }

    const auto *Record = SuperClassBaseType->getAs<RecordType>();
    if (!Record)
      continue;

    auto *Decl = Record->getDecl();
    assert(Decl && "Unexpected Program State");
    assert(isa<CXXRecordDecl>(Decl) && "Unexpected Program State");

    if (isDescendantOf(llvm::dyn_cast_or_null<CXXRecordDecl>(Decl),
                       AncestorName))
      return true;
  }
  return false;
}

const CXXMethodDecl *
GoogleTestExtractor::findTestMethod(const CXXRecordDecl *TestCXXRecordDecl,
                                    std::string TestMethodName) {
  std::vector<CXXMethodDecl *> TestMethods =
      util::findCXXMethodDeclFromCXXRecordDecls(TestCXXRecordDecl,
                                                TestMethodName);
  if (!TestMethods.empty())
    return TestMethods.back();
  return nullptr;
}

const CXXMethodDecl *GoogleTestExtractor::findTestMethodRecursively(
    const CXXRecordDecl *ClassCXXRecordDecl, std::string TargetMethodName) {
  if (!ClassCXXRecordDecl)
    return nullptr;

  const CXXMethodDecl *TestMethod =
      findTestMethod(ClassCXXRecordDecl, TargetMethodName);
  if (TestMethod)
    return TestMethod;

  for (auto SuperClassBaseSpecifier : ClassCXXRecordDecl->bases()) {
    auto *SuperClassCXXRecordDecl = cast<CXXRecordDecl>(
        SuperClassBaseSpecifier.getType()->getAs<RecordType>()->getDecl());
    TestMethod =
        findTestMethodRecursively(SuperClassCXXRecordDecl, TargetMethodName);
    if (TestMethod)
      return TestMethod;
  }

  return nullptr;
}

const std::vector<const NamedDecl *>
GoogleTestExtractor::getTestSequence(const clang::NamedDecl *Test) {
  std::vector<const NamedDecl *> SequenceNamedDecls;
  const auto *TestCXXRecordDecl = cast<CXXRecordDecl>(Test);

  std::vector<std::string> TestSequence = {SetupTestcaseName, SetupName,
                                           TestName, TeardownName,
                                           TeardownTestcaseName};

  for (std::string ActionName : TestSequence) {
    const auto *ActionDecl =
        findTestMethodRecursively(TestCXXRecordDecl, ActionName);
    if (ActionDecl)
      SequenceNamedDecls.emplace_back(ActionDecl);
  }
  return SequenceNamedDecls;
}

bool GoogleTestExtractor::isTestBody(const llvm::Function *Function) const {
  std::string MethodFullName = util::getDemangledName(Function->getName());
  return (util::getMethodName(MethodFullName).compare(TestName) == 0);
}

const std::vector<Unittest>
GoogleTestExtractor::getTestcasesFromASTUnit(const llvm::Module &M,
                                             ASTUnit &ASTUnit) {

  const std::string RecordDeclTag = "RecordDeclTag";
  const std::string TestMatcherTag = "Test";

  std::vector<Unittest> UTs;
  std::vector<std::string> MatcherTags = {RecordDeclTag, TestMatcherTag};
  DeclarationMatcher RecordDeclMatcher =
      cxxRecordDecl(hasDefinition(), isExpansionInMainFile())
          .bind(RecordDeclTag);
  DeclarationMatcher TestMatcher =
      cxxRecordDecl(matchesName(".*_Test"),
                    has(cxxMethodDecl(hasName(TestName))),
                    unless(has(cxxMethodDecl(hasName("AddToRegistry")))))
          .bind(TestMatcherTag);
  MatchFinder MatchFinder;
  NamedDeclCallback MatchCallback(MatcherTags);
  MatchFinder.addMatcher(TestMatcher, &MatchCallback);
  MatchFinder.addMatcher(RecordDeclMatcher, &MatchCallback);
  MatchFinder.matchAST(ASTUnit.getASTContext());
  auto Tests = MatchCallback.getFoundNamedDecls(TestMatcherTag);
  std::vector<const NamedDecl *> Environments;
  for (const auto *Decl : MatchCallback.getFoundNamedDecls(RecordDeclTag)) {
    if (isDescendantOf(llvm::dyn_cast_or_null<CXXRecordDecl>(Decl),
                       "Environment")) {
      Environments.emplace_back(Decl);
    }
  }

  std::vector<std::pair<FunctionDecl *, const llvm::Function *>> EnvSetUps;
  std::vector<std::pair<FunctionDecl *, const llvm::Function *>> EnvTearDowns;
  for (const auto *EnvClass : Environments) {
    for (auto *EnvMethod : cast<CXXRecordDecl>(EnvClass)->methods()) {
      std::string MethodName = EnvMethod->getNameAsString();
      if (MethodName.compare(SetupName) == 0) {
        EnvSetUps.emplace_back(
            cast<FunctionDecl>(EnvMethod),
            util::getIRFunction(M, cast<NamedDecl>(EnvMethod)));
      } else if (MethodName.compare(TeardownName) == 0) {
        EnvTearDowns.emplace_back(
            cast<FunctionDecl>(EnvMethod),
            util::getIRFunction(M, cast<NamedDecl>(EnvMethod)));
      }
    }
  }
  for (const auto *Test : Tests) {
    if (!Test)
      continue;

    std::vector<FunctionNode> TestSequence;
    for (const auto &TestStepNamedDecl : getTestSequence(Test)) {
      const llvm::Function *TestStepFunction =
          util::getIRFunction(M, TestStepNamedDecl);
      if (!TestStepFunction)
        continue;
      TestSequence.emplace_back(*TestStepFunction,
                                isTestBody(TestStepFunction));
    }
    for (auto SetUp = EnvSetUps.rbegin(); SetUp != EnvSetUps.rend(); SetUp++) {
      assert(SetUp->second && "Unexpected Program State");
      TestSequence.emplace(TestSequence.begin(),
                           FunctionNode(*SetUp->second, false, true));
    }
    for (auto TearDown = EnvTearDowns.rbegin(); TearDown != EnvTearDowns.rend();
         TearDown++) {
      assert(TearDown->second && "Unexpected Program State");
      TestSequence.emplace(TestSequence.begin(),
                           FunctionNode(*TearDown->second, false, true));
    }
    UTs.emplace_back(*Test, "gtest", TestSequence);
  }
  return UTs;
}

std::vector<Unittest> GoogleTestExtractor::extractTCs() {
  std::vector<Unittest> Unittests;
  const auto &M = SrcCollection.getLLVMModule();
  for (const auto &ASTUnit : SrcCollection.getASTUnits()) {
    auto UTs = getTestcasesFromASTUnit(M, *ASTUnit);
    Unittests.insert(Unittests.end(), UTs.begin(), UTs.end());
  }
  return Unittests;
}
