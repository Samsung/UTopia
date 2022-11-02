#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/AssignUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "testutil/SourceFileManager.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Tooling/Tooling.h"

using namespace ftg::util;
using namespace clang;
using namespace ast_matchers;

namespace ftg {

class TestASTUtil : public ::testing::Test {
protected:
  std::unique_ptr<ASTIRMap> ASTMap;
  std::unique_ptr<IRAccessHelper> IRAccess;
  std::unique_ptr<UTLoader> Loader;

  ASTDefNode *getADN(std::string FuncName, unsigned BIdx, unsigned IIdx,
                     int OIdx) {
    if (!IRAccess || !ASTMap)
      return nullptr;

    auto *I = IRAccess->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return nullptr;
    if (OIdx != -1 && (unsigned)OIdx >= I->getNumOperands())
      return nullptr;
    return ASTMap->getASTDefNode(*I, OIdx);
  }

  const ASTNode *getRValueASTNode(std::string FuncName, unsigned BIdx,
                                  unsigned IIdx, int OIdx) {
    auto *ADN = getADN(FuncName, BIdx, IIdx, OIdx);
    if (!ADN)
      return nullptr;

    return ADN->getAssigned();
  }

  const Expr *findCallExpr(ASTUnit &Unit, std::string CallerName,
                           std::string CalleeName) const {
    const std::string Tag = "Tag";
    std::vector<const Expr *> Exprs;
    auto &Ctx = Unit.getASTContext();
    auto Matcher =
        expr(
            anyOf(callExpr(hasAncestor(functionDecl(hasName(CallerName))),
                           callee(functionDecl(hasName(CalleeName)))),
                  cxxConstructExpr(
                      hasAncestor(functionDecl(hasName(CallerName))),
                      hasDeclaration(cxxConstructorDecl(hasName(CalleeName)))),
                  cxxDeleteExpr(hasAncestor(functionDecl(hasName(CallerName)))),
                  cxxMemberCallExpr(
                      hasAncestor(functionDecl(hasName(CallerName))),
                      callee(functionDecl(hasName(CalleeName)))),
                  cxxNewExpr(hasAncestor(functionDecl(hasName(CallerName)))),
                  cxxOperatorCallExpr(
                      hasAncestor(functionDecl(hasName(CallerName))),
                      callee(functionDecl(hasName(CalleeName))))))
            .bind(Tag);
    for (auto &Node : match(Matcher, Ctx)) {
      const auto *Record = Node.getNodeAs<Expr>(Tag);
      if (!Record)
        continue;
      Exprs.push_back(Record);
    }
    if (Exprs.size() != 1)
      return nullptr;
    return Exprs[0];
  }

  const Expr *findAssignOperator(ASTUnit &Unit, std::string CallerName) const {
    const std::string Tag = "Tag";
    std::vector<const Expr *> Exprs;
    auto &Ctx = Unit.getASTContext();
    auto Matcher =
        binaryOperator(hasAncestor(functionDecl(hasName(CallerName))),
                       isAssignmentOperator())
            .bind(Tag);
    for (auto &Node : match(Matcher, Ctx)) {
      const auto *Record = Node.getNodeAs<Expr>(Tag);
      if (!Record)
        continue;
      Exprs.push_back(Record);
    }
    if (Exprs.size() != 1)
      return nullptr;
    return Exprs[0];
  }

  bool load(std::string SrcDir, std::string CodePath) {
    return load(SrcDir, std::vector<std::string>({CodePath}));
  }

  bool load(std::string SrcDir, std::vector<std::string> CodePaths) {
    auto CH = TestHelperFactory().createCompileHelper(
        SrcDir, CodePaths, "-O0 -g -w", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    Loader =
        std::make_unique<UTLoader>(CH, nullptr, std::vector<std::string>());
    if (!Loader)
      return false;

    const auto &SC = Loader->getSourceCollection();
    IRAccess = std::make_unique<IRAccessHelper>(SC.getLLVMModule());
    if (!IRAccess)
      return false;

    ASTMap = std::make_unique<DebugInfoMap>(SC);
    if (!ASTMap)
      return false;

    return true;
  }
};

FunctionDecl *getFunctionDecl(ASTUnit &AST, std::string FuncName) {
  for (auto D : AST.getASTContext().getTranslationUnitDecl()->decls()) {
    if (auto clangF = D->getAsFunction()) {
      if (clangF->getName() != FuncName)
        continue;
      return clangF;
    }
  }
  return nullptr;
}

class TestStripConstExpr : public ::testing::Test {

protected:
  static std::unique_ptr<ASTUnit> AST;

  static void SetUpTestCase() {

    AST = tooling::buildASTFromCode(
        "typedef int const_int;\n"
        "void func(\n"
        "    const_int* P0, const_int *P1, const_int * P2,\n"
        "    const const_int* P3, const const_int *P4, const const_int * P5,\n"
        "    const_int* const P6, const_int *const P7, const_int * const P8,\n"
        "    const_int const* P9, const_int const *P10,\n"
        "    const_int const * P11, const_int const P12,\n"
        "    const const_int arg13) {}");
  }
};

std::unique_ptr<ASTUnit> TestStripConstExpr::AST = nullptr;

TEST_F(TestStripConstExpr, P) {

  ASSERT_TRUE(AST);
  FunctionDecl *F = getFunctionDecl(*AST, "func");
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(3)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(4)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(5)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(6)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(7)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(8)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(9)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(10)->getType().getAsString()));
  EXPECT_EQ("const_int *",
            stripConstExpr(F->getParamDecl(11)->getType().getAsString()));
  EXPECT_EQ("const_int",
            stripConstExpr(F->getParamDecl(12)->getType().getAsString()));
  EXPECT_EQ("const_int",
            stripConstExpr(F->getParamDecl(13)->getType().getAsString()));
}
TEST_F(TestStripConstExpr, N) {

  ASSERT_TRUE(AST);
  FunctionDecl *F = getFunctionDecl(*AST, "func");
  std::string TypeStr0 = F->getParamDecl(0)->getType().getAsString();
  std::string TypeStr1 = F->getParamDecl(1)->getType().getAsString();
  std::string TypeStr2 = F->getParamDecl(2)->getType().getAsString();
  EXPECT_EQ(TypeStr0, stripConstExpr(TypeStr0));
  EXPECT_EQ(TypeStr1, stripConstExpr(TypeStr1));
  EXPECT_EQ(TypeStr2, stripConstExpr(TypeStr2));
}

TEST(TestASTUtils, getTypeNameP) {

  std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
      "typedef struct A {} AA;\n"
      "typedef struct {} BB;\n"
      "void func(\n"
      "    struct A* P0, struct A *P1, struct A * P2, const struct A* P3,\n"
      "    const struct A *P4, const struct A * P5, struct A* const P6,\n"
      "    struct A *const P7, struct A * const P8, struct A const* P9,\n"
      "    struct A const *P10, struct A const * P11, AA* P12, AA *P13,\n"
      "    AA * P14, const AA* P15, const AA *P16, const AA * P17,\n"
      "    AA* const P18, AA *const P19, AA * const P20, AA const* P21,\n"
      "    AA const *P22, AA const * P23);\n"
      "void func2(\n"
      "    BB* P0, BB *P1, BB * P2, const BB* P3, const BB *P4,\n"
      "    const BB * P5, BB* const P6, BB *const P7, BB * const P8,\n"
      "    BB const* P9, BB const *P10, BB const * P11);\n");

  ASSERT_TRUE(AST);

  for (auto Arg : getFunctionDecl(*AST, "func")->parameters()) {
    QualType Ty = Arg->getType();
    while (Ty->isPointerType()) {
      Ty = Ty->getPointeeType();
    }
    EXPECT_EQ("A", getTypeName(*Ty->getAsTagDecl()));
  }
  for (auto Arg : getFunctionDecl(*AST, "func2")->parameters()) {
    QualType Ty = Arg->getType();
    while (Ty->isPointerType()) {
      Ty = Ty->getPointeeType();
    }
    EXPECT_EQ("BB", getTypeName(*Ty->getAsTagDecl()));
  }
}

TEST(TestASTUtils, getAsArrayTypeP) {

  std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
      "int VarLen = 5;\n"
      "void func(int P0[], int P1[10], int P2[VarLen]) {}");

  ASSERT_TRUE(AST);

  auto *F = getFunctionDecl(*AST, "func");
  F->dump(llvm::outs());
  for (auto A : F->parameters()) {
    EXPECT_TRUE(getAsArrayType(A->getType()));
    EXPECT_TRUE(getAsArrayType(A->getOriginalType()));
  }
}

TEST(TestASTUtils, getAsArrayTypeN) {

  std::unique_ptr<ASTUnit> AST = tooling::buildASTFromCode(
      "typedef struct A {} AA;\n"
      "void func(int a0, int *a1, int **a2, struct A a3, AA a4) {}");

  ASSERT_TRUE(AST);

  auto *F = getFunctionDecl(*AST, "func");
  for (auto A : F->parameters()) {
    EXPECT_FALSE(getAsArrayType(A->getType()));
    EXPECT_FALSE(getAsArrayType(A->getOriginalType()));
  }
}

TEST_F(TestASTUtil, collectConstructorsP) {
  const std::string Code1 = "class CLS1 {\n"
                            "public:\n"
                            "  static CLS1 *create(int P1, int P2) {\n"
                            "    return new CLS1(P1, P2);\n"
                            "}\n"
                            "  CLS1() = default;\n"
                            "  CLS1(int P) : F1(P), F2(0) {}\n"
                            "private:\n"
                            "  int F1;\n"
                            "  int F2;\n"
                            "  CLS1(int P1, int P2) : F1(P1), F2(P2) {}\n"
                            "};\n"
                            "void test1() {\n"
                            "  CLS1 Var1;\n"
                            "  CLS1 Var2(1);\n"
                            "  CLS1 *Var3 = CLS1::create(2, 3);\n"
                            "}\n";
  const std::string Code2 = "class CLS2 {\n"
                            "public:\n"
                            "  CLS2() : F(0) {}\n"
                            "private:\n"
                            "  int F;\n"
                            "};\n"
                            "void test2() { CLS2 Var1; }\n";
  SourceFileManager SFM;
  SFM.createFile("test1.cpp", Code1);
  SFM.createFile("test2.cpp", Code2);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), {SFM.getFilePath("test1.cpp"),
                                         SFM.getFilePath("test2.cpp")}));
  ASSERT_TRUE(Loader);

  std::set<std::string> Answers = {
      "_ZN4CLS1C2Ev",    "_ZN4CLS1C1Ev",    "_ZN4CLS1C1Ei",
      "_ZN4CLS1C2Ei",    "_ZN4CLS1C2Eii",   "_ZN4CLS1C1Eii",
      "_ZN4CLS1C2ERKS_", "_ZN4CLS1C1ERKS_", "_ZN4CLS1C2EOS_",
      "_ZN4CLS1C1EOS_",  "_ZN4CLS2C2Ev",    "_ZN4CLS2C1Ev",
      "_ZN4CLS2C2ERKS_", "_ZN4CLS2C1ERKS_", "_ZN4CLS2C1EOS_",
      "_ZN4CLS2C2EOS_"};
  for (const auto *Constructor :
       util::collectConstructors(Loader->getSourceCollection().getASTUnits())) {
    ASSERT_TRUE(Constructor);
    for (auto &MangledName : util::getMangledNames(*Constructor)) {
      auto Iter = Answers.find(MangledName);
      ASSERT_NE(Iter, Answers.end());
      Answers.erase(Iter);
    }
  }
  ASSERT_TRUE(Answers.empty());
}

TEST_F(TestASTUtil, collectConstructorsN) {
  const std::string Code = "void test() {\n"
                           "  int Var;\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));
  ASSERT_TRUE(Loader);

  auto Constructors = util::collectConstructors({nullptr});
  ASSERT_EQ(Constructors.size(), 0);

  Constructors =
      util::collectConstructors(Loader->getSourceCollection().getASTUnits());
  ASSERT_EQ(Constructors.size(), 0);
}

TEST_F(TestASTUtil, collectNonStaticMethodsP) {
  const std::string Code1 = "class CLS1 {\n"
                            "public:\n"
                            "  void M1();\n"
                            "private:\n"
                            "  void M2() const;\n"
                            "};\n"
                            "void test1() { CLS1 Var; }\n";
  const std::string Code2 = "class CLS2 {\n"
                            "public:\n"
                            "  void M1();\n"
                            "};\n"
                            "void test2() { CLS2 Var; }\n";
  SourceFileManager SFM;
  SFM.createFile("test1.cpp", Code1);
  SFM.createFile("test2.cpp", Code2);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), {SFM.getFilePath("test1.cpp"),
                                         SFM.getFilePath("test2.cpp")}));
  ASSERT_TRUE(Loader);

  std::set<std::string> Answers = {
      "_ZN4CLS12M1Ev",   "_ZNK4CLS12M2Ev",  "_ZN4CLS1C2Ev",   "_ZN4CLS1C1Ev",
      "_ZN4CLS1C2ERKS_", "_ZN4CLS1C1ERKS_", "_ZN4CLS1C2EOS_", "_ZN4CLS1C1EOS_",
      "_ZN4CLS22M1Ev",   "_ZN4CLS2C2Ev",    "_ZN4CLS22M1Ev",  "_ZN4CLS2C1Ev",
      "_ZN4CLS2C2ERKS_", "_ZN4CLS2C1ERKS_", "_ZN4CLS2C2EOS_", "_ZN4CLS2C1EOS_"};
  for (const auto *Method : util::collectNonStaticClassMethods(
           Loader->getSourceCollection().getASTUnits())) {
    ASSERT_TRUE(Method);
    for (auto &MangledName : util::getMangledNames(*Method)) {
      auto Iter = Answers.find(MangledName);
      ASSERT_NE(Iter, Answers.end());
      Answers.erase(Iter);
    }
  }
  ASSERT_TRUE(Answers.empty());
}

TEST_F(TestASTUtil, collectNonStaticMethodsN) {
  const std::string Code = "static void F1();\n"
                           "void F2();\n"
                           "class CLS {\n"
                           "public:\n"
                           "  static void M1();\n"
                           "};\n"
                           "void test() { CLS Var; F1(); F2(); }\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));
  ASSERT_TRUE(Loader);

  auto Methods = util::collectNonStaticClassMethods({nullptr});
  ASSERT_EQ(Methods.size(), 0);

  std::set<std::string> Answers = {"_ZN3CLSC2Ev",    "_ZN3CLSC1Ev",
                                   "_ZN3CLSC2ERKS_", "_ZN3CLSC1ERKS_",
                                   "_ZN3CLSC2EOS_",  "_ZN3CLSC1EOS_"};
  for (const auto *Method : util::collectNonStaticClassMethods(
           Loader->getSourceCollection().getASTUnits())) {
    ASSERT_TRUE(Method);
    for (auto &MangledName : util::getMangledNames(*Method)) {
      auto Iter = Answers.find(MangledName);
      ASSERT_NE(Iter, Answers.end());
      Answers.erase(Iter);
    }
  }
  ASSERT_TRUE(Answers.empty());
}

TEST_F(TestASTUtil, GetArgASTsAndFunctionDeclP) {
  auto AST = tooling::buildASTFromCode(
      "extern \"C\" {\n"
      "class CLS1 {\n"
      "public:\n"
      "  CLS1();\n"
      "  void M1();\n"
      "  void M2(int P);\n"
      "  void M3(int P1, int P2);\n"
      "};\n"
      "class CLS2 { public: CLS2(int P); };\n"
      "class CLS3 { public: CLS3(int P1, int P2); };\n"
      "void F1();\n"
      "void F2(int P);\n"
      "void F3(int P1, int P2);\n"
      "void test_callexpr() {\n"
      "  F1();\n"
      "  F2(1);\n"
      "  F3(1, 2);\n"
      "}\n"
      "void test_constructexpr() {\n"
      "  CLS1 Var1;\n"
      "  CLS2 Var2(1);\n"
      "  CLS3 Var3(1, 2);\n"
      "}\n"
      "void test_deleteexpr() {\n"
      "  int *Var1;\n"
      "  delete Var1;\n"
      "}\n"
      "void test_membercallexpr() {\n"
      "  CLS1 Var;\n"
      "  Var.M1();\n"
      "  Var.M2(1);\n"
      "  Var.M3(1, 2);\n"
      "}\n"
      "void test_newexpr1() {\n"
      "  int *Var;\n"
      "  Var = new int;\n"
      "}\n"
      "void test_newexpr2() {\n"
      "  int *Var;\n"
      "  Var = new int(10);\n"
      "}\n"
      "}");
  ASSERT_TRUE(AST);

  auto Verifier = [&AST, this](std::string CallerName, std::string CalleeName,
                               unsigned AnswerArgNum) -> bool {
    if (!AST)
      return false;
    auto *E = findCallExpr(*AST, CallerName, CalleeName);
    if (!E)
      return false;
    if (getArgExprs(*const_cast<Expr *>(E)).size() != AnswerArgNum)
      return false;
    if (!util::getFunctionDecl(*const_cast<Expr *>(E)))
      return false;
    return true;
  };

  ASSERT_TRUE(Verifier("test_callexpr", "F1", 0));
  ASSERT_TRUE(Verifier("test_callexpr", "F2", 1));
  ASSERT_TRUE(Verifier("test_callexpr", "F3", 2));
  ASSERT_TRUE(Verifier("test_constructexpr", "CLS1", 0));
  ASSERT_TRUE(Verifier("test_constructexpr", "CLS2", 1));
  ASSERT_TRUE(Verifier("test_constructexpr", "CLS3", 2));
  ASSERT_TRUE(Verifier("test_deleteexpr", "", 1));
  ASSERT_TRUE(Verifier("test_membercallexpr", "M1", 0));
  ASSERT_TRUE(Verifier("test_membercallexpr", "M2", 1));
  ASSERT_TRUE(Verifier("test_membercallexpr", "M3", 2));
  ASSERT_TRUE(Verifier("test_newexpr1", "", 0));
  ASSERT_TRUE(Verifier("test_newexpr2", "", 1));
}

TEST_F(TestASTUtil, GetArgASTsAndFunctionDeclN) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void test_noncallexpr() {\n"
                                       "  int Var;\n"
                                       "  Var = 1;\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  auto *E = findAssignOperator(*AST, "test_noncallexpr");
  ASSERT_TRUE(E);
  ASSERT_THROW(getArgExprs(*const_cast<Expr *>(E)), std::invalid_argument);
  ASSERT_FALSE(util::getFunctionDecl(*const_cast<Expr *>(E)));
}

TEST_F(TestASTUtil, GetDebugLocP) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "class CLS {\n"
                                       "public:\n"
                                       "  void M1();\n"
                                       "  bool operator==(int P);\n"
                                       "};\n"
                                       "void test_expr() {\n"
                                       "  int Var;\n"
                                       "  Var = 1;\n"
                                       "}\n"
                                       "void test_membercallexpr() {\n"
                                       "  CLS Var;\n"
                                       "  Var.M1();\n"
                                       "}\n"
                                       "bool test_operatorcallexpr() {\n"
                                       "  CLS Var;\n"
                                       "  return Var == 1;\n"
                                       "}\n"
                                       "}\n");
  ASSERT_TRUE(AST);
  const auto &SrcManager = AST->getSourceManager();

  const Expr *E;
  SourceLocation Loc;

  E = findAssignOperator(*AST, "test_expr");
  ASSERT_TRUE(E);
  Loc = getDebugLoc(*E);
  ASSERT_EQ(SrcManager.getExpansionLineNumber(Loc), 9);
  ASSERT_EQ(SrcManager.getExpansionColumnNumber(Loc), 3);

  E = findCallExpr(*AST, "test_membercallexpr", "M1");
  ASSERT_TRUE(E);
  Loc = getDebugLoc(*E);
  ASSERT_EQ(SrcManager.getExpansionLineNumber(Loc), 13);
  ASSERT_EQ(SrcManager.getExpansionColumnNumber(Loc), 7);

  E = findCallExpr(*AST, "test_operatorcallexpr", "operator==");
  ASSERT_TRUE(E);
  Loc = getDebugLoc(*E);
  ASSERT_EQ(SrcManager.getExpansionLineNumber(Loc), 17);
  ASSERT_EQ(SrcManager.getExpansionColumnNumber(Loc), 14);
}

TEST_F(TestASTUtil, GetMacroFunctionExpansionRangeP) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void API(int P1);\n"
                                       "#define MACRO(Var) API(Var)\n"
                                       "void test() {\n"
                                       "  int Var;\n"
                                       "  MACRO(Var);\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const auto *E = findCallExpr(*AST, "test", "API");
  ASSERT_TRUE(E);

  auto Range =
      getMacroFunctionExpansionRange(AST->getSourceManager(), E->getBeginLoc());
  ASSERT_TRUE(Range.isValid());
}

TEST_F(TestASTUtil, GetMacroFunctionExpansionRange_InvalidLocN) {
  auto AST = tooling::buildASTFromCode("");
  ASSERT_TRUE(AST);

  auto Range =
      getMacroFunctionExpansionRange(AST->getSourceManager(), SourceLocation());
  ASSERT_TRUE(Range.isInvalid());
}

TEST_F(TestASTUtil, GetMacroFunctionExpansionRange_NonFunctionMacroN) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "#define MACRO(Var) Var = 1\n"
                                       "void test() {\n"
                                       "  int Var;\n"
                                       "  MACRO(Var);\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const auto *E = findAssignOperator(*AST, "test");
  ASSERT_TRUE(E);

  auto Range =
      getMacroFunctionExpansionRange(AST->getSourceManager(), E->getBeginLoc());
  ASSERT_TRUE(Range.isInvalid());
}

TEST_F(TestASTUtil, GetMacroFunctionExpansionRange_NonMacroN) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void test() {\n"
                                       "  int Var;\n"
                                       "  Var = 1;\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const auto *E = findAssignOperator(*AST, "test");
  ASSERT_TRUE(E);

  auto Range =
      getMacroFunctionExpansionRange(AST->getSourceManager(), E->getBeginLoc());
  ASSERT_TRUE(Range.isInvalid());
}

TEST_F(TestASTUtil, IsDefaultArgumentP) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "class CLS {\n"
                                       "public:\n"
                                       "  void set(int P = 10) { F = P; }\n"
                                       "private:\n"
                                       "  int F;\n"
                                       "};\n"
                                       "void test_default() {\n"
                                       "  CLS Var;\n"
                                       "  Var.set();\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  auto *E = findCallExpr(*AST, "test_default", "set");
  ASSERT_TRUE(E);
  ASSERT_TRUE(isDefaultArgument(*const_cast<Expr *>(E), 0));
}

TEST_F(TestASTUtil, IsDefaultArgumentN) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void API();\n"
                                       "void test_invalid_aidx() {\n"
                                       "  API();\n"
                                       "}\n"
                                       "void test_noncallexpr() {\n"
                                       "  int Var;\n"
                                       "  Var = 1;\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const Expr *E;

  E = findCallExpr(*AST, "test_invalid_aidx", "API");
  ASSERT_TRUE(E);
  ASSERT_ANY_THROW(isDefaultArgument(*const_cast<Expr *>(E), 0));

  E = findAssignOperator(*AST, "test_noncallexpr");
  ASSERT_TRUE(E);
  ASSERT_ANY_THROW(isDefaultArgument(*const_cast<Expr *>(E), 0));
}

TEST_F(TestASTUtil, IsImplicitArgumentP) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void test_new1() {\n"
                                       "  int *Var1 = new int;\n"
                                       "}\n"
                                       "void test_new2() {\n"
                                       "  int *Var2 = new int();\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const Expr *E;

  E = findCallExpr(*AST, "test_new1", "");
  ASSERT_TRUE(E);
  ASSERT_TRUE(isImplicitArgument(*const_cast<Expr *>(E), 0));

  E = findCallExpr(*AST, "test_new2", "");
  ASSERT_TRUE(E);
  ASSERT_TRUE(isImplicitArgument(*const_cast<Expr *>(E), 0));
}

TEST_F(TestASTUtil, IsImplicitArgumentN) {
  auto AST = tooling::buildASTFromCode("extern \"C\" {\n"
                                       "void test_noncallexpr() {\n"
                                       "  int Var;\n"
                                       "  Var = 1;\n"
                                       "}\n"
                                       "}");
  ASSERT_TRUE(AST);

  const Expr *E;

  E = findAssignOperator(*AST, "test_noncallexpr");
  ASSERT_TRUE(E);
  ASSERT_ANY_THROW(isImplicitArgument(*const_cast<Expr *>(E), 0));
}

TEST_F(TestASTUtil, IsMacroArgUsedHashHashExpansionP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int, int);\n"
                           "void SUB_0(int P1, int P2) {\n"
                           "  API(P1, P2);\n"
                           "}\n"
                           "#define MACRO(P1, P2) SUB_##P1(P1, P2)\n"
                           "void test() {\n"
                           "  MACRO(0, 1);\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));

  const auto *RValueNode = getRValueASTNode("test", 0, 0, 0);
  ASSERT_TRUE(RValueNode);

  auto *E = RValueNode->getNode().get<clang::Expr>();
  ASSERT_TRUE(E);
  ASSERT_TRUE(
      util::isMacroArgUsedHashHashExpansion(*E, RValueNode->getASTUnit()));
}

TEST_F(TestASTUtil, IsMacroArgUsedHashHashExpansionN) {
  const std::string Code = "extern \"C\" {\n"
                           "#define MACRO1(P) P * 10\n"
                           "#define MACRO2(P) #P\n"
                           "void API1(int);\n"
                           "void API2(char []);\n"
                           "void test() {\n"
                           "  API1(MACRO1(10));\n"
                           "  API2(MACRO2(Hello World));\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));

  const ASTNode *RValueNode;
  const clang::Expr *E;

  RValueNode = getRValueASTNode("test", 0, 0, 0);
  ASSERT_TRUE(RValueNode);
  E = RValueNode->getNode().get<clang::Expr>();
  ASSERT_TRUE(E);
  ASSERT_FALSE(
      util::isMacroArgUsedHashHashExpansion(*E, RValueNode->getASTUnit()));

  RValueNode = getRValueASTNode("test", 0, 1, 0);
  ASSERT_TRUE(RValueNode);
  E = RValueNode->getNode().get<clang::Expr>();
  ASSERT_TRUE(E);
  ASSERT_FALSE(
      util::isMacroArgUsedHashHashExpansion(*E, RValueNode->getASTUnit()));
}

} // end namespace ftg
