#include "ftg/tcanalysis/TCTExtractor.h"
#include "ftg/utils/ASTUtil.h"

#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTUnit.h"

#include <string>

using namespace ftg;
using namespace clang;

TCTExtractor::TCTExtractor(const SourceCollection &SC) : TCExtractor(SC) {
  for (const auto &ASTUnit : SrcCollection.getASTUnits()) {
    parseAST(ASTUnit->getASTContext());
    TestProject.matchExtraAction();
  }
}

std::vector<Unittest> TCTExtractor::extractTCs() {
  std::vector<Unittest> Unittests;
  const auto &TCArr = TestProject.getTCArray();

  for (const auto &UTFunc : TestProject.getUTFuncs()) {
    if (!TCArr.getTestcase(UTFunc.getName()))
      continue;

    auto TestSequence = getTestStepsFromUTFuncBody(UTFunc);
    Location Loc;
    const auto &Decl = UTFunc.getFunctionDecl();
    Loc.setLocation(Decl.getASTContext().getSourceManager(),
                    Decl.getSourceRange());
    Unittests.emplace_back(Decl, Loc, "tct", TestSequence);
  }
  return Unittests;
}

std::vector<FunctionNode>
TCTExtractor::getTestStepsFromUTFuncBody(const UTFuncBody &U) {
  const llvm::Module &M = SrcCollection.getLLVMModule();
  std::vector<FunctionNode> TestSteps;

  Testcase *TC = TestProject.getTCArray().getTestcase(U.getName());
  if (!TC)
    return TestSteps;

  auto TestSequence = {U.getStartupName(), U.getName(), U.getCleanupName()};
  for (std::string ActionName : TestSequence) {
    if (ActionName.empty())
      continue;
    bool IsTestBody = (ActionName.compare(U.getName()) == 0);
    const auto *F = M.getFunction(ActionName);
    assert(F && "Unexpected Program State");
    TestSteps.emplace_back(*F, IsTestBody);
  }
  return TestSteps;
}

class TCArrayVisitor : public RecursiveASTVisitor<TCArrayVisitor> {
public:
  TCArrayVisitor(Project &TestProject) : TestProject(TestProject) {}

  bool VisitVarDecl(VarDecl *VisitedVarDecl) {
    if (dyn_cast_or_null<ConstantArrayType>(
            VisitedVarDecl->getType().getTypePtr())) {
      TCArrayFlag = (VisitedVarDecl->getName().compare(TCArrayName) == 0);
    }
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *VisitedDeclRefExpr) {
    if (TCArrayFlag) {
      std::string VisitedDeclName =
          VisitedDeclRefExpr->getNameInfo().getName().getAsString();

      auto &TCArr = TestProject.getTCArray();
      if (VisitedDeclName.find(StartupName) != std::string::npos) {
        TCArr.getCurTestcase()->setStartupName(VisitedDeclName);
      } else if (VisitedDeclName.find(CleanupName) != std::string::npos) {
        TCArr.getCurTestcase()->setCleanupName(VisitedDeclName);
      } else {
        TCArr.addTestcase(VisitedDeclName);
      }
    }
    return true;
  }

private:
  Project &TestProject;
  bool TCArrayFlag = false;
  const std::string TCArrayName = "tc_array";
  const std::string StartupName = "startup";
  const std::string CleanupName = "cleanup";
};

void TCTExtractor::setTCArray(clang::ASTContext &Context) {
  TCArrayVisitor Visitor(TestProject);
  Visitor.TraverseDecl(Context.getTranslationUnitDecl());
}

void TCTExtractor::parseAST(clang::ASTContext &Context) {
  setTCArray(Context);

  SourceManager &SM = Context.getSourceManager();
  for (auto *D : Context.getTranslationUnitDecl()->decls()) {
    if (FunctionDecl *F = llvm::dyn_cast<FunctionDecl>(D)) {
      if (!F->isDefined())
        continue;
      if (!SM.isInMainFile(F->getBeginLoc()))
        continue;
      if (F->isMain())
        continue;
      if (!UTFuncBody::isUT(F->getNameAsString()))
        continue;
      TestProject.addUTFunc(F);
    }
  }
}
