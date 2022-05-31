#ifndef FTG_TCANALYSIS_GOOGLETESTEXTRACTOR_H
#define FTG_TCANALYSIS_GOOGLETESTEXTRACTOR_H

#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/TCExtractor.h"

#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/Module.h"

#include <string>
#include <vector>

namespace ftg {

class GoogleTestExtractor : public TCExtractor {
public:
  GoogleTestExtractor(const SourceCollection &SC);

private:
  bool isDescendantOf(const clang::CXXRecordDecl *ClassCXXRecordDecl,
                      std::string AncestorName);
  const clang::CXXMethodDecl *
  findTestMethod(const clang::CXXRecordDecl *TestCXXRecordDecl,
                 std::string TestMethodName);
  const clang::CXXMethodDecl *
  findTestMethodRecursively(const clang::CXXRecordDecl *ClassCXXRecordDecl,
                            std::string TargetMethodName);
  const std::vector<const clang::NamedDecl *>
  getTestSequence(const clang::NamedDecl *Test);
  bool isTestBody(const llvm::Function *Function) const;
  const std::vector<Unittest> getTestcasesFromASTUnit(const llvm::Module &M,
                                                      clang::ASTUnit &ASTUnit);
  std::vector<Unittest> extractTCs() override;

  const std::string TestName = "TestBody";
  const std::string SetupName = "SetUp";
  const std::string TeardownName = "TearDown";
  const std::string SetupTestcaseName = "SetUpTestCase";
  const std::string TeardownTestcaseName = "TearDownTestCase";
};

} // namespace ftg

#endif // FTG_TCANALYSIS_GOOGLETESTEXTRACTOR_H
