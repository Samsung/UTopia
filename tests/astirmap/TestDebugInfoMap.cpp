#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "testutil/IRAccessHelper.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;

class TestDebugInfoMap : public ::testing::Test {
protected:
  std::unique_ptr<IRAccessHelper> AccessHelper;
  std::unique_ptr<DebugInfoMap> Map;
  std::unique_ptr<SourceCollection> SC;

  ASTDefNode *getASTDefNode(std::string FuncName, unsigned BIdx, unsigned IIdx,
                            int AIdx) {
    if (!AccessHelper || !Map)
      return nullptr;

    auto *I = AccessHelper->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return nullptr;

    return Map->getASTDefNode(*I, AIdx);
  }

  bool load(std::string BaseDirPath, std::string CodePath) {
    return load(BaseDirPath, std::vector<std::string>({CodePath}));
  }

  ASTDefNode *getASTDefNode(std::string GlobalVarName) {
    if (!AccessHelper)
      return nullptr;

    auto *GVar = AccessHelper->getGlobalVariable(GlobalVarName);
    if (!GVar)
      return nullptr;

    return Map->getASTDefNode(*GVar);
  }

  bool load(std::string BaseDirPath, std::vector<std::string> CodePaths) {
    auto CH = TestHelperFactory().createCompileHelper(
        BaseDirPath, CodePaths, "-O0 -g -w", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    if (!SC)
      return false;

    AccessHelper = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!AccessHelper)
      return false;

    Map = std::make_unique<DebugInfoMap>(*SC);
    if (!Map)
      return false;
    return true;
  }

  bool verifyLocation(ASTDefNode &ADN, unsigned BaseLine, unsigned BaseColumn,
                      unsigned LHSLine, unsigned LHSColumn, unsigned LHSLength,
                      unsigned RHSLine, unsigned RHSColumn,
                      unsigned RHSLength) const {
    if (!verifyLocation(ADN, BaseLine, BaseColumn, LHSLine, LHSColumn,
                        LHSLength))
      return false;

    auto *Assigned = ADN.getAssigned();
    if (!Assigned)
      return false;

    const auto &Index = Assigned->getIndex();
    if (Index.getLine() != RHSLine)
      return false;
    if (Index.getColumn() != RHSColumn)
      return false;
    if (Assigned->getLength() != RHSLength)
      return false;

    return true;
  }

  bool verifyLocation(ASTDefNode &ADN, unsigned BaseLine, unsigned BaseColumn,
                      unsigned LHSLine, unsigned LHSColumn,
                      unsigned LHSLength) const {
    auto BaseLoc = ADN.getLocIndex();
    if (BaseLoc.getLine() != BaseLine)
      return false;
    if (BaseLoc.getColumn() != BaseColumn)
      return false;

    auto &Assignee = ADN.getAssignee();
    const auto &Index = Assignee.getIndex();
    if (Index.getLine() != LHSLine)
      return false;
    if (Index.getColumn() != LHSColumn)
      return false;
    if (Assignee.getLength() != LHSLength)
      return false;

    return true;
  }
};

TEST_F(TestDebugInfoMap, getDef_ArgumentP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int P1, int P2);\n"
                           "class CLS {\n"
                           "public:\n"
                           "  CLS() = default;\n"
                           "  CLS(int P) : F(P) {}\n"
                           "private:\n"
                           "  int F;\n"
                           "};\n"
                           "void test_call() {\n"
                           "  API(1, 2);\n"
                           "}\n"
                           "void test_constructor() {\n"
                           "  CLS *Var1 = new CLS(1);\n"
                           "  CLS Var2(2);\n"
                           "}\n"
                           "void test_new_delete(void *P) {\n"
                           "  CLS *Var1 = new CLS(10);\n"
                           "  CLS *Var2 = new CLS[10];\n"
                           "  delete Var1;\n"
                           "  delete[] Var2;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_call", 0, 0, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 11, 7, 11, 3, 9, 11, 7, 1));

  ADN = getASTDefNode("test_call", 0, 0, 1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 11, 10, 11, 3, 9, 11, 10, 1));

  ADN = getASTDefNode("test_constructor", 0, 7, 1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 14, 23, 14, 19, 6, 14, 23, 1));

  ADN = getASTDefNode("test_constructor", 1, 2, 1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 15, 12, 15, 7, 7, 15, 12, 1));

  ADN = getASTDefNode("test_new_delete", 0, 10, 1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 18, 23, 18, 19, 7, 18, 23, 2));

  ADN = getASTDefNode("test_new_delete", 1, 2, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 19, 23, 19, 15, 11, 19, 23, 2));

  ADN = getASTDefNode("test_new_delete", 2, 1, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 20, 10, 20, 3, 11, 20, 10, 4));

  ADN = getASTDefNode("test_new_delete", 4, 1, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 21, 12, 21, 3, 13, 21, 12, 4));
}

TEST_F(TestDebugInfoMap, getDef_ArgumentN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int P1, int P2);\n"
                           "class CLS {\n"
                           "public:\n"
                           "  CLS(int P = 10) : F(P) {}\n"
                           "  bool operator<(int P) const { return F < P; }\n"
                           "private:\n"
                           "  int F;\n"
                           "};\n"
                           "void test_default() {\n"
                           "  CLS Var;\n"
                           "}\n"
                           "void test_implicit() {\n"
                           "  int *Var1 = new int;\n"
                           "  int *Var2 = new int();\n"
                           "}\n"
                           "bool test_operator() {\n"
                           "  CLS Var;\n"
                           "  return Var < 1;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASSERT_FALSE(getASTDefNode("test_default", 0, 2, 1));
  ASSERT_FALSE(getASTDefNode("test_implicit", 0, 3, 0));
  ASSERT_FALSE(getASTDefNode("test_implicit", 0, 7, 0));
  ASSERT_FALSE(getASTDefNode("test_operator", 0, 2, 1));
}

TEST_F(TestDebugInfoMap, getDef_AssignP) {
  const std::string Code = "extern \"C\" {\n"
                           "class CLS {\n"
                           "};\n"
                           "void test_assign() {\n"
                           "  int Var1, Var2;\n"
                           "  Var1 = 1;\n"
                           "  Var1 = Var2 = 2;\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_assign", 0, 4, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 6, 8, 6, 3, 4, 6, 10, 1));

  ADN = getASTDefNode("test_assign", 0, 5, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 7, 15, 7, 10, 4, 7, 17, 1));

  ADN = getASTDefNode("test_assign", 0, 6, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 7, 8, 7, 3, 4, 7, 17, 1));
}

TEST_F(TestDebugInfoMap, getDef_ConstP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int);\n"
                           "const int Const1 = 1;\n"
                           "const int Const2 = Const1;\n"
                           "class CLS {\n"
                           "public:\n"
                           "  CLS() : F1(Const1) {}\n"
                           "private:\n"
                           "  int F1;\n"
                           "};\n"
                           "void test_const() {\n"
                           "  int Var1 = Const1;\n"
                           "  int Var2 = Const2;\n"
                           "  int Var3;\n"
                           "  Var3 = Const1;\n"
                           "  API(Const1);\n"
                           "  CLS Var4;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_const", 0, 5, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 11, 3, 11, 10, 3, 20, 1));

  ADN = getASTDefNode("test_const", 0, 7, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 11, 3, 11, 10, 3, 20, 1));

  ADN = getASTDefNode("test_const", 0, 9, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 11, 3, 11, 10, 3, 20, 1));

  ADN = getASTDefNode("test_const", 0, 10, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 11, 3, 11, 10, 3, 20, 1));

  ADN = getASTDefNode("_ZN3CLSC2Ev", 0, 5, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 11, 3, 11, 10, 3, 20, 1));
}

TEST_F(TestDebugInfoMap, getDef_CtorP) {
  const std::string Code = "extern \"C\" {\n"
                           "class CLS {\n"
                           "public:\n"
                           "  CLS(int P1, int P2) : F1(P1), F2(P2) {}\n"
                           "private:\n"
                           "  int F1;\n"
                           "  int F2;\n"
                           "};\n"
                           "void test_ctor() {\n"
                           "  CLS Var(1, 2);\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_ctor", 0, 2, 1);
  ASSERT_TRUE(ADN);
  EXPECT_TRUE(verifyLocation(*ADN, 10, 11, 10, 7, 9, 10, 11, 1));

  ADN = getASTDefNode("test_ctor", 0, 2, 2);
  ASSERT_TRUE(ADN);
  EXPECT_TRUE(verifyLocation(*ADN, 10, 14, 10, 7, 9, 10, 14, 1));
}

TEST_F(TestDebugInfoMap, getDef_MacroP) {
  const std::string Code = "extern \"C\" {\n"
                           "#define MACRO_1(Var) sizeof(Var) / sizeof(Var[0])\n"
                           "#define MACRO_2(Var) Var; Var\n"
                           "#define MACRO_3(Var) API_1(Var)\n"
                           "void API_1(int);\n"
                           "void test_macro() {\n"
                           "  API_1(__LINE__);\n"
                           "  char Var1[] = \"Hello\";\n"
                           "  API_1(MACRO_1(Var1));\n"
                           "  MACRO_2(API_1(10));\n"
                           "  MACRO_3(30);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_macro", 0, 1, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 7, 9, 7, 3, 15, 7, 9, 8));

  ADN = getASTDefNode("test_macro", 0, 5, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 9, 9, 9, 3, 20, 9, 9, 13));

  ADN = getASTDefNode("test_macro", 0, 6, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 10, 3, 10, 11, 9, 10, 17, 2));

  ADN = getASTDefNode("test_macro", 0, 7, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 10, 3, 10, 11, 9, 10, 17, 2));

  ADN = getASTDefNode("test_macro", 0, 8, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 11, 3, 11, 3, 11, 11, 11, 2));
}

TEST_F(TestDebugInfoMap, getDef_MacroN) {
  const std::string Code = "extern \"C\" {\n"
                           "#define MACRO_1 API_2(API_2(10))\n"
                           "#define MACRO_2(Var1, Var2) Var1; Var2\n"
                           "#define MACRO_3(Var) \\\n"
                           "do {\\\n"
                           "  const int Var1 = 0;\\\n"
                           "  const int Var2 = 1;\\\n"
                           "  Var(Var1, Var2);\\\n"
                           "} while(0);\n"
                           "void API_1();\n"
                           "int API_2(int);\n"
                           "void API_3(int, int);\n"
                           "void test_macro() {\n"
                           "  MACRO_1;\n"
                           "  MACRO_2(API_1(), API_1());\n"
                           "  MACRO_3(API_3);\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  // Note:
  // API_1 that is in MACRO_1 expansion body should not be found because
  // there are more than one function call for API_1 in same macro body.
  ASSERT_FALSE(getASTDefNode("test_macro", 0, 0, -1));
  ASSERT_FALSE(getASTDefNode("test_macro", 0, 1, -1));

  // Note:
  // If same call instructions do not have same macro caller location,
  // they should not be identified because called function name is not
  // unique to map IR and AST.
  ASSERT_FALSE(getASTDefNode("test_macro", 0, 2, -1));
  ASSERT_FALSE(getASTDefNode("test_macro", 0, 3, -1));

  // Note:
  // If arguments are constant variable and all of them are from macro body
  // expansion. ASTDefNode should not be identified because there is no way
  // to distinguish tokens in the macro.
  ASSERT_FALSE(getASTDefNode("test_macro", 1, 4, 0));
  ASSERT_FALSE(getASTDefNode("test_macro", 1, 4, 1));
}

TEST_F(TestDebugInfoMap, getDef_TemplateTypeN) {
  const std::string Code = "template <typename T> class CLS {\n"
                           "public:\n"
                           "  CLS() : F(0) {}\n"
                           "private:\n"
                           "  T F;\n"
                           "};\n"
                           "extern \"C\" {\n"
                           "void test_identify_template_type_field() {\n"
                           "  CLS <int> Var1;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));
  ASSERT_FALSE(getASTDefNode("_ZN4CLS1IiEC2Ev", 0, 5, -1));
}

TEST_F(TestDebugInfoMap, getDef_VarDeclP) {
  const std::string Code1 = "extern \"C\" {\n"
                            "static int GVar1 = 1;\n"
                            "extern const  int GVar2 = 2;\n"
                            "int GVar3, GVar4 = 3;\n"
                            "void API1(int);\n"
                            "void test_local() {\n"
                            "  int Var1 = 1;\n"
                            "  const  int Var2 = 2;\n"
                            "  int Var3, Var4 = 3;\n"
                            "}\n"
                            "void test_staticlocal() {\n"
                            "  static int Var1 = 1;\n"
                            "  static const  int Var2 = 2;\n"
                            "  static int Var3, Var4 = 3;\n"
                            "  API1(Var1);\n"
                            "  API1(Var2);\n"
                            "  API1(Var3);\n"
                            "  API1(Var4);\n"
                            "}\n"
                            "void test_global() {\n"
                            "  API1(GVar1);\n"
                            "  API1(GVar2);\n"
                            "  API1(GVar3);\n"
                            "  API1(GVar4);\n"
                            "}}\n";
  const std::string Code2 = "extern \"C\" {\n"
                            "static int GVar1 = 1;\n"
                            "void API1(int);\n"
                            "void test_duplicate() {\n"
                            "  API1(GVar1);\n"
                            "}}\n";
  SourceFileManager SFM;
  SFM.createFile("test1.cpp", Code1);
  SFM.createFile("test2.cpp", Code2);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), {SFM.getFilePath("test1.cpp"),
                                          SFM.getFilePath("test2.cpp")}));

  ASTDefNode *ADN;

  ADN = getASTDefNode("test_local", 0, 5, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 7, 7, 7, 7, 8, 7, 14, 1));

  ADN = getASTDefNode("test_local", 0, 7, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 8, 14, 8, 14, 8, 8, 21, 1));

  ADN = getASTDefNode("test_local", 0, 2, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 9, 7, 9, 7, 4));

  ADN = getASTDefNode("test_local", 0, 10, -1);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 9, 13, 9, 13, 8, 9, 20, 1));

  ADN = getASTDefNode("_ZZ16test_staticlocalE4Var1");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 12, 14, 12, 14, 8, 12, 21, 1));

  ADN = getASTDefNode("test_staticlocal", 0, 2, 0);
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 13, 21, 13, 21, 8, 13, 28, 1));

  ADN = getASTDefNode("_ZZ16test_staticlocalE4Var3");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 14, 14, 14, 14, 4));

  ADN = getASTDefNode("_ZZ16test_staticlocalE4Var4");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 14, 20, 14, 20, 8, 14, 27, 1));

  ADN = getASTDefNode("_ZL5GVar1");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 2, 12, 2, 12, 9, 2, 20, 1));

  ADN = getASTDefNode("GVar2");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 3, 19, 3, 19, 9, 3, 27, 1));

  ADN = getASTDefNode("GVar3");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 4, 5, 4, 5, 5));

  ADN = getASTDefNode("GVar4");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 4, 12, 4, 12, 9, 4, 20, 1));

  ADN = getASTDefNode("_ZL5GVar1.1");
  ASSERT_TRUE(ADN);
  ASSERT_TRUE(verifyLocation(*ADN, 2, 12, 2, 12, 9, 2, 20, 1));
}

TEST_F(TestDebugInfoMap, getDiffNumArgsP) {
  const std::string Code =
      "extern \"C\" {\n"
      "struct ST { int F1; int F2; int F3; int F4; int F5; int F6; };\n"
      "class CLS {\n"
      "public:\n"
      "  int M1() const;\n"
      "  ST M2() const;\n"
      "  static int M3();\n"
      "};\n"
      "ST F1();\n"
      "int F2();\n"
      "void test() {\n"
      "  CLS Var;\n"
      "  Var.M1();\n"
      "  Var.M2();\n"
      "  CLS::M3();\n"
      "  F1();\n"
      "  F2();\n"
      "}\n"
      "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  llvm::CallBase *CB;

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test", 0, 4));
  ASSERT_TRUE(CB);
  ASSERT_EQ(Map->getDiffNumArgs(*CB), 1);

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test", 0, 5));
  ASSERT_TRUE(CB);
  ASSERT_EQ(Map->getDiffNumArgs(*CB), 2);

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test", 0, 6));
  ASSERT_TRUE(CB);
  ASSERT_EQ(Map->getDiffNumArgs(*CB), 0);

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test", 0, 7));
  ASSERT_TRUE(CB);
  ASSERT_EQ(Map->getDiffNumArgs(*CB), 1);

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test", 0, 8));
  ASSERT_TRUE(CB);
  ASSERT_EQ(Map->getDiffNumArgs(*CB), 0);
}

TEST_F(TestDebugInfoMap, getDiffNumArgsN) {
  const std::string Code = "extern \"C\" {\n"
                           "struct ST { int F1; int F2; };\n"
                           "void test_callnotfound() {\n"
                           "  struct ST Var = { 1, 2 };\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  llvm::CallBase *CB;

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test_callnotfound", 0, 3));
  ASSERT_TRUE(CB);
  ASSERT_THROW(Map->getDiffNumArgs(*CB), std::runtime_error);
}

TEST_F(TestDebugInfoMap, hasDiffNumArgsN) {
  const std::string Code = "extern \"C\" {\n"
                           "struct ST { int F1; int F2; };\n"
                           "void test_callnotfound() {\n"
                           "  struct ST Var = { 1, 2 };\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  llvm::CallBase *CB;

  CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      AccessHelper->getInstruction("test_callnotfound", 0, 3));
  ASSERT_TRUE(CB);
  ASSERT_THROW(Map->hasDiffNumArgs(*CB), std::runtime_error);
}

TEST_F(TestDebugInfoMap, ConstructorWithImplicitValueP) {
  const std::string Code = "extern \"C\" {"
                           "class A {"
                           "  public:"
                           "    A() : cnt() {}"
                           "  private:"
                           "    int cnt;"
                           "};"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  // Check whether implicit initializer cnt() is properly ignored.
  // Creating DebugInfoMap fails if it is not ignored.
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));
}

TEST_F(TestDebugInfoMap, ArgumentsInMacroN) {
  std::string Code = "extern \"C\" {\n"
                     "#define MACRO 0, 0\n"
                     "void API(int, int, int, int);\n"
                     "void test() {\n"
                     "  API(0, MACRO, 0);"
                     "}\n"
                     "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));
  llvm::errs() << SC->getLLVMModule() << "\n";

  auto *I = AccessHelper->getInstruction("test", 0, 0);
  ASSERT_TRUE(I);

  auto *ADN = Map->getASTDefNode(*I, 0);
  ASSERT_TRUE(ADN);

  ADN = Map->getASTDefNode(*I, 1);
  ASSERT_FALSE(ADN);

  ADN = Map->getASTDefNode(*I, 2);
  ASSERT_FALSE(ADN);

  ADN = Map->getASTDefNode(*I, 3);
  ASSERT_TRUE(ADN);
}
