#include "TestHelper.h"
#include "ftg/rootdefanalysis/RDAnalyzer.h"
#include "ftg/rootdefanalysis/RDNode.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"

using namespace llvm;
using namespace ftg;

class TestRDAnalyzer : public TestBase {
protected:
  std::set<RDNode> find(std::string FuncName, unsigned BIdx, unsigned IIdx,
                        unsigned OIdx, std::vector<std::string> FuncsToExplore,
                        RDExtension *Extension) {
    if (!IRAH)
      return {};

    auto *I = IRAH->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return {};

    if (I->getNumOperands() <= OIdx)
      return {};

    const auto &U = I->getOperandUse(OIdx);

    std::vector<llvm::Function *> Funcs;
    for (const auto &Func : FuncsToExplore) {
      auto *F = IRAH->getFunction(Func);
      if (!F)
        return {};

      Funcs.push_back(F);
    }

    RDAnalyzer Analyzer(0, Extension);
    Analyzer.setSearchSpace(Funcs);
    return Analyzer.getRootDefinitions(U);
  }
};

TEST_F(TestRDAnalyzer, NoResultHandleRegisterN) {
  std::string Code = "extern \"C\" {\n"
                     "int sub2(int P) { return P; }\n"
                     "int sub1(int P) {\n"
                     "  int V1 = P;\n"
                     "  return sub2(V1);"
                     "}\n"
                     "void test() {\n"
                     "  int V1 = 1;\n"
                     "  int V2 = sub1(V1);\n"
                     "}\n"
                     "}\n";
  ASSERT_TRUE(loadCPP(Code));

  auto Nodes = find("test", 0, 7, 0, {"test"}, nullptr);
  ASSERT_EQ(Nodes.size(), 1);

  auto *I = IRAH->getInstruction("test", 0, 3);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());
}

TEST_F(TestRDAnalyzer, SameRouteDiffTargetP) {
  std::string Code = "extern \"C\" {\n"
                     "int test() {\n"
                     "  int V1 = 10;\n"
                     "  int V2 = 0;\n"
                     "  int i = 0;\n"
                     "  while (i++ < 10) {\n"
                     "    if (i < 5) {\n"
                     "      V2 += V1;\n"
                     "    }\n"
                     "  }\n"
                     "  return V2;\n"
                     "}}";
  ASSERT_TRUE(loadCPP(Code));

  auto Nodes = find("test", 5, 1, 0, {"test"}, nullptr);
  ASSERT_EQ(Nodes.size(), 2);

  auto *I = IRAH->getInstruction("test", 0, 4);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());

  I = IRAH->getInstruction("test", 0, 6);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());
}

TEST_F(TestRDAnalyzer, NonGlobalOrArgContinueP) {
  std::string Code = "extern \"C\" {\n"
                     "int sub2(int p) { return p * 10; }\n"
                     "int sub1() {\n"
                     "  int V = 0;\n"
                     "  int i = 0;\n"
                     "  while (i++ < 10) {\n"
                     "    if (i < 5) {\n"
                     "      V += sub2(V);\n"
                     "    }\n"
                     "  }\n"
                     "  return V;\n"
                     "}\n"
                     "void test() {\n"
                     "  int V = sub1();\n"
                     "}}\n";
  ASSERT_TRUE(loadCPP(Code));

  auto Nodes = find("test", 0, 3, 0, {"test"}, nullptr);
  ASSERT_EQ(Nodes.size(), 1);

  auto *I = IRAH->getInstruction("sub1", 0, 3);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());
}

TEST_F(TestRDAnalyzer, RecursionFunctionVisitP) {
  std::string Code = "extern \"C\" {\n"
                     "void api(int);\n"
                     "int test(int *p) {\n"
                     "  while (*p < 10) {\n"
                     "    *p += test(p);\n"
                     "    api(*p);\n"
                     "  }\n"
                     "  return *p;\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code));

  // NOTE: This test may run forever if visit mechanism is malfunction.
  auto Nodes = find("test", 2, 8, 0, {"test"}, nullptr);
  ASSERT_EQ(Nodes.size(), 1);

  auto *I = IRAH->getInstruction("test", 0, 1);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());
}

TEST_F(TestRDAnalyzer, RecursionFunctionVisitN) {
  std::string Code = "extern \"C\" {\n"
                     "void api(int);\n"
                     "int sub(int P) {\n"
                     "  if (P == 0) return 0;\n"
                     "  return sub(P-1);\n"
                     "}\n"
                     "void test() {\n"
                     "  api(sub(10));\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code));

  // NOTE: This test may run forever if visit mechanism is malfunction.
  auto Nodes = find("test", 0, 1, 0, {"test"}, nullptr);
  ASSERT_EQ(Nodes.size(), 1);
  auto *I = IRAH->getInstruction("sub", 1, 0);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());
}

TEST_F(TestRDAnalyzer, MethodInvocationVisitP) {
  std::string Code = "extern \"C\" {\n"
                     "class CLS {\n"
                     "public:\n"
                     "  void M(int, CLS *);\n"
                     "};\n"
                     "void api(CLS *);\n"
                     "void test() {\n"
                     "  CLS V1;\n"
                     "  int V2 = 10;\n"
                     "  for (int i = 0; i < 10; ++i) {\n"
                     "    V1.M(V2, &V1);\n"
                     "  }\n"
                     "  api(&V1);\n"
                     "}}\n";
  ASSERT_TRUE(loadCPP(Code));

  RDExtension Extension;
  for (const auto *Method :
       util::collectNonStaticClassMethods(SC->getASTUnits())) {
    ASSERT_TRUE(Method);
    for (auto &MangledName : util::getMangledNames(*Method)) {
      Extension.addNonStaticClassMethod(MangledName);
    }
  }

  // NOTE: This test may run forever if visit mechanism is malfunction.
  auto Nodes = find("test", 4, 0, 0, {"test"}, &Extension);
  ASSERT_EQ(Nodes.size(), 2);

  auto *I = IRAH->getInstruction("test", 0, 5);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(0, *I)), Nodes.end());

  auto *V = IRAH->getInstruction("test", 0, 0);
  ASSERT_TRUE(V);
  I = IRAH->getInstruction("test", 0, 5);
  ASSERT_TRUE(I);
  ASSERT_NE(Nodes.find(RDNode(*V, *I)), Nodes.end());
}

#define BUILD(Timeout, Extension, ...)                                         \
  auto Analyzer = std::make_unique<RDAnalyzer>(Timeout, Extension);            \
  do {                                                                         \
    std::vector<std::string> Names = {__VA_ARGS__};                            \
    std::vector<llvm::Function *> Funcs;                                       \
    for (auto Name : Names) {                                                  \
      auto *F = IRAH->getFunction(Name);                                       \
      ASSERT_NE(F, nullptr);                                                   \
      Funcs.push_back(F);                                                      \
    }                                                                          \
    Analyzer->setSearchSpace(Funcs);                                           \
  } while (0)

#define ADD_OUT_PARAM(Var)                                                     \
  do {                                                                         \
    RDExtension Extension;                                                     \
    for (auto &Pair : Var) {                                                   \
      Extension.addOutParam(Pair.first, Pair.second);                          \
    }                                                                          \
    Analyzer.setExtension(Extension);                                          \
  } while (0)

#define FIND(FIdx, BIdx, IIdx, AIdx)                                           \
  std::set<RDNode> RootDefs;                                                   \
  do {                                                                         \
    auto *I = IRAH->getInstruction(FIdx, BIdx, IIdx);                          \
    ASSERT_NE(I, nullptr);                                                     \
    RootDefs = Analyzer->getRootDefinitions(I->getOperandUse(AIdx));           \
  } while (0)

#define VERIFY_NUM(Num) ASSERT_EQ(RootDefs.size(), Num)

#define VERIFY_INST(FIdx, BIdx, IIdx, ArgIdx)                                  \
  do {                                                                         \
    auto *I = IRAH->getInstruction(FIdx, BIdx, IIdx);                          \
    ASSERT_NE(I, nullptr);                                                     \
    bool Found = false;                                                        \
    for (auto &RootDef : RootDefs) {                                           \
      if (exist(RootDef, *I, ArgIdx)) {                                        \
        Found = true;                                                          \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    ASSERT_TRUE(Found);                                                        \
  } while (0)

#define VERIFY_GLOBAL(Name, ArgIdx)                                            \
  do {                                                                         \
    auto *G = IRAH->getGlobalVariable(Name);                                   \
    ASSERT_NE(G, nullptr);                                                     \
    bool Found = false;                                                        \
    for (auto &RootDef : RootDefs) {                                           \
      if (exist(RootDef, *G, ArgIdx)) {                                        \
        Found = true;                                                          \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    ASSERT_TRUE(Found);                                                        \
  } while (0)

bool exist(const RDNode &Node, llvm::Value &V, int Idx) {
  auto Result = Node.getDefinition();
  return Result.first == &V && Result.second == Idx;
}

class TestRootDefAnalyzer : public TestBase {};

TEST_F(TestRootDefAnalyzer, DefinitionsP) {
  const char *CODE =
      "extern \"C\" {\n"
      "typedef struct _ST1 {\n"
      "  long long Field0;\n"
      "  long long Field1;\n"
      "  int Field2;\n"
      "} ST1;\n"
      "int GVar1 = 30;\n"
      "ST1 GVar2 = { .Field0 = 10, .Field1 = 20, .Field2 = 30};\n"
      "int SUB_1() {\n"
      "  return 10;\n"
      "}\n"
      "void SUB_2(int P1, int *P2) {\n"
      "  *P2 = P1;\n"
      "}\n"
      "void API_1(int);\n"
      "int API_2(int);\n"
      "void API_3(const char*);\n"
      "void API_4(int (*fp)(int));\n"
      "void API_5(ST1* P1);\n"
      "void test() {\n"
      "  int Var1 = 0;\n"
      "  API_1(Var1);\n"
      "  int Var2 = 0;\n"
      "  Var2 = 10;\n"
      "  API_1(Var2);\n"
      "  int Var3 = API_2(Var1);\n"
      "  API_1(Var3);\n"
      "  API_1(SUB_1());\n"
      "  API_1(GVar1);\n"
      "  API_3(\"Hello World\");\n"
      "  API_4(API_2);\n"
      "  ST1 Var4 = { 0, 1, 2 };\n"
      "  API_5(&Var4);\n"
      "  Var4.Field2 = 10;\n"
      "  API_1(Var4.Field2);\n"
      "  API_5(&GVar2);\n"
      "}\n"
      "void test_arg_from_tracing() {\n"
      "  int Var1 = 10;\n"
      "  int *Var2 = &Var1;\n"
      "  SUB_2(10, Var2);\n"
      "  API_1(*Var2);\n"
      "}\n"
      "}\n";

  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test");
  {
    // API_1(Var1) -> int Var1 = 0 (Local Value Delcaration)
    FIND("test", 0, 7, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 5, -1);
  }

  {
    // API_2(Var2) -> Var2 = 10 (Reassignment)
    FIND("test", 0, 12, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 10, -1);
  }

  {
    // API_1(Var3) -> int Var1 = 0 (External Function Return)
    FIND("test", 0, 18, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 5, -1);
  }

  {
    // API_1(SUB_1()) -> return 10; (Internal Function Return)
    FIND("test", 0, 20, 0);
    VERIFY_NUM(1);
    VERIFY_INST("SUB_1", 0, 0, -1);
  }

  {
    // API_1(GVar1) -> int GVar1 = 30; (Global Variable Declaration)
    FIND("test", 0, 22, 0);
    VERIFY_NUM(1);
    VERIFY_GLOBAL("GVar1", -1);
  }

  {
    // API_3("Hello World") -> "Hello World" (GEP Constant In Call)
    FIND("test", 0, 23, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 23, 0);
  }

  {
    // API_4(API_2); -> API_2 (Function Pointer In Call)
    FIND("test", 0, 24, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 24, 0);
  }

  {
    // API_5(&Var5); -> ST1 Var5 = { 0, 1, 2 }
#if LLVM_VERSION_MAJOR < 17
    FIND("test", 0, 28, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 27, -1);
#else
    FIND("test", 0, 27, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 26, -1);
#endif
  }

  {
    // API_1(Var4.Field2) -> Var4.Field2 = 10
#if LLVM_VERSION_MAJOR < 17
    FIND("test", 0, 33, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 30, -1);
#else
    FIND("test", 0, 32, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 29, -1);
#endif
  }

  {
    // API_5(&GVar2) -> ST1 GVar2 = { .Field0 = 10, ...
#if LLVM_VERSION_MAJOR < 17
    FIND("test", 0, 34, 0);
#else
    FIND("test", 0, 33, 0);
#endif
    VERIFY_NUM(1);
    VERIFY_GLOBAL("GVar2", -1);
  }

  {
    // API_1(*Var2) in test_arg_from_tracing -> 1st arg of SUB_2(10, Var2)
    BUILD(0, nullptr, "test_arg_from_tracing");
    FIND("test_arg_from_tracing", 0, 10, 0);
    VERIFY_NUM(1);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_INST("test_arg_from_tracing", 0, 7, 0);
#endif
  }
}

TEST_F(TestRootDefAnalyzer, PathP) {
  const char *CODE = "extern \"C\" {\n"
                     "int GVar1 = 30;\n"
                     "void API_1(int);\n"
                     "void SUB_1(int* P1) {\n"
                     "  if (*P1 == 5) {\n"
                     "    *P1 = 30;\n"
                     "  } else if(*P1 == 10) {\n"
                     "    GVar1 = 50;\n"
                     "  }\n"
                     "}\n"
                     "int SUB_2(int *P1) {\n"
                     "  return *P1;\n"
                     "}\n"
                     "void SUB_3(int *P1);\n"
                     "void SUB_3(int *P1) {\n"
                     "  if (*P1 == 10) {\n"
                     "     *P1 = 20;\n"
                     "     return;\n"
                     "  }\n"
                     "  *P1 -= 1;\n"
                     "  SUB_3(P1);\n"
                     "}\n"
                     "void test_1() {\n"
                     "  GVar1 = 100;\n"
                     "}\n"
                     "void test_2() {\n"
                     "  for (int Var1 = 0; Var1 < 10; ++Var1) {\n"
                     "    API_1(Var1);"
                     "  }\n"
                     "  int Var2 = 10;\n"
                     "  SUB_1(&Var2);\n"
                     "  API_1(Var2);\n"
                     "  SUB_1(&GVar1);\n"
                     "  API_1(GVar1);\n"
                     "}\n"
                     "void test_samefunc_difftarget() {\n"
                     "  int Var1 = 10;\n"
                     "  int Var2 = 100;\n"
                     "  Var1 = SUB_2(&Var2);\n"
                     "  Var1 = SUB_2(&Var2);\n"
                     "  API_1(Var1);\n"
                     "}\n"
                     "void test_recursion() {\n"
                     "  int Var1 = 10;\n"
                     "  SUB_3(&Var1);\n"
                     "  API_1(Var1);\n"
                     "}\n"
                     "}\n";
  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test_1", "test_2");

  {
    // API_1(Var1) -> for (int Var1 = 10, ...)
    FIND("test_2", 2, 1, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test_2", 0, 3, -1);
  }

  {
    // API_1(Var2) -> int Var2 = 10
    //             -> *P1 = 30
    FIND("test_2", 4, 4, 0);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_NUM(2);
#endif
    VERIFY_INST("test_2", 4, 1, -1);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_INST("SUB_1", 1, 1, -1);
#endif
  }

  {
    // API_1(GVar1) -> int GVar1 = 30
    //              -> *P1 = 30
    //              -> GVar1 = 50
    FIND("test_2", 4, 7, 0);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_NUM(3);
    VERIFY_INST("SUB_1", 1, 1, -1);
    VERIFY_INST("SUB_1", 3, 0, -1);
    VERIFY_INST("test_1", 0, 0, -1);
#endif
  }

  {
    BUILD(0, nullptr, "test_samefunc_difftarget");
    FIND("test_samefunc_difftarget", 0, 11, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test_samefunc_difftarget", 0, 5, -1);
  }

  {
    BUILD(0, nullptr, "test_recursion");
    FIND("test_recursion", 0, 5, 0);
    for (auto &RD : RootDefs)
      llvm::outs() << RD << "\n";
    VERIFY_NUM(1);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_INST("SUB_3", 1, 1, -1);
#endif
  }
}

bool exist(std::set<RDNode> &Nodes, Value &A, int Idx) {
  auto Acc =
      std::find_if(Nodes.begin(), Nodes.end(), [&A, Idx](const auto &Node) {
        auto Def = Node.getDefinition();
        return (Def.first == &A) && (Def.second == Idx);
      });

  return Acc != Nodes.end();
}

bool addOutParam(DirectionAnalysisReport &DirectionReport,
                 const llvm::Module &M, std::string FuncName, unsigned ArgIdx) {
  const auto *F = M.getFunction(FuncName);
  if (!F)
    return false;

  const auto *A = F->getArg(ArgIdx);
  if (!A)
    return false;

  DirectionReport.set(*A, Dir_Out);
  return true;
}

class TestDFAnalyzer : public TestBase {};

TEST_F(TestDFAnalyzer, ExternalP) {
  const char *CODE =
      "extern \"C\" {\n"
      "typedef struct _ST1 { long long a; long long b; long long c; } ST1;\n"
      "int GVar1;\n"
      "int EXTERN_1(int, int*);\n"
      "int EXTERN_2(int*);\n"
      "int EXTERN_3();\n"
      "void EXTERN_4(int*, int*);\n"
      "void API_1(int, int*);\n"
      "void API_2(int);\n"
      "void API_3(int);\n"
      "void API_4(int);\n"
      "void API_5(int);\n"
      "void API_6(int);\n"
      "void API_7(int*);\n"
      "ST1 API_8();\n"
      "ST1 API_9(int*, int);\n"
      "void test() {\n"
      "  int Var1 = 10;\n"
      "  int Var2;\n"
      "  int Var3 = EXTERN_1(Var1, &Var2);\n"
      "  int Var4;\n"
      "  API_1(Var3, &Var4);\n"
      "  Var1 = EXTERN_2(&Var1);\n"
      "  API_2(Var1);\n"
      "  Var1 = EXTERN_3();\n"
      "  API_3(Var1);\n"
      "  API_4(Var2);\n"
      "  Var1 = EXTERN_2(&Var2);\n"
      "  API_5(Var2);\n"
      "  EXTERN_4(&Var2, &Var2);\n"
      "  API_6(Var2);\n"
      "  API_7(&GVar1);\n"
      "  ST1 Var5 = API_8();\n"
      "  Var5 = API_9(&Var1, GVar1);\n"
      "}}";

  ASSERT_TRUE(loadCPP(CODE));

  auto DirectionReport = std::make_shared<DirectionAnalysisReport>();
  ASSERT_TRUE(
      addOutParam(*DirectionReport, SC->getLLVMModule(), "EXTERN_1", 1));
  ASSERT_TRUE(
      addOutParam(*DirectionReport, SC->getLLVMModule(), "EXTERN_2", 0));
  ASSERT_TRUE(
      addOutParam(*DirectionReport, SC->getLLVMModule(), "EXTERN_4", 0));
  ASSERT_TRUE(
      addOutParam(*DirectionReport, SC->getLLVMModule(), "EXTERN_4", 1));
  ASSERT_TRUE(addOutParam(*DirectionReport, SC->getLLVMModule(), "API_1", 1));
  ASSERT_TRUE(addOutParam(*DirectionReport, SC->getLLVMModule(), "API_7", 0));
  ASSERT_TRUE(addOutParam(*DirectionReport, SC->getLLVMModule(), "API_9", 1));

  RDExtension Extension;
  Extension.setDirectionReport(DirectionReport);
  BUILD(0, &Extension, "test");

  {
    // 1st Argument of API_1(Var3, &Var4)
    //   -> int Var1 = 10;
    FIND("test", 0, 15, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 7, -1);
  }

  {
    // 2nd Argument of API_1(Var3, &Var4)
    //   -> int Var1 = 10;
    //   -> 2nd Argument of EXTERN_1(Var1, &Var2)
    FIND("test", 0, 15, 1);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 7, -1);
  }

  {
    // 1st Argument of API_2(Var1)
    //   -> EXTERN_2(&Var1);
    FIND("test", 0, 19, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 16, -1);
  }

  {
    // 1st Argument of API_3(Var1)
    //   -> EXTERN_3();
    FIND("test", 0, 23, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 20, -1);
  }

  {
    // 1st Argument of API_4(Var2)
    //   -> int Var1 = 10;
    FIND("test", 0, 25, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 7, -1);
  }

  {
    // 1st Argument of API_5(Var2)
    //   -> 1st Argument of EXTERN_2(&Var2)
    FIND("test", 0, 29, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 26, 0);
  }

  {
    // 1st Argument of API_6(Var2)
    //   -> 1st Argument of EXTERN_4(&Var2, &Var2)
    //   -> 2nd Argument of EXTERN_4(&Var2, &Var2)
    FIND("test", 0, 32, 0);
    VERIFY_NUM(2);
    VERIFY_INST("test", 0, 30, 0);
    VERIFY_INST("test", 0, 30, 1);
  }

  {
    // 1st Argument of API_7(&GVar1)
    //   -> 1st Argument of API_7(&GVar1)
    FIND("test", 0, 33, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 33, 0);
  }

  {
    // Return of API_8() (SRet)
    //   -> Return of API_8()
    FIND("test", 0, 35, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 35, 0);
  }

  {
    // Return of API_9() (SRet)
    //   -> 2nd Argument of API_7(&GVar1)
    FIND("test", 0, 37, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 33, 0);
  }
}

TEST_F(TestRootDefAnalyzer, 1N) {
  RDAnalyzer Analyzer;
  try {
    Analyzer.setSearchSpace({nullptr});
    ASSERT_TRUE(1);
  } catch (std::invalid_argument &Exc) {
  }
}

TEST_F(TestRootDefAnalyzer, UninitializedN) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(int*);\n"
                     "void test() {\n"
                     "  int Var1[10];\n"
                     "  API_1(Var1);\n"
                     "}}\n";
  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test");

  {
    FIND("test", 0, 3, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 0, -1);
  }
}

TEST_F(TestRootDefAnalyzer, NoPrevInstN) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(int**);\n"
                     "void test() {\n"
                     "  int* Var1;\n"
                     "  API_1(&Var1);\n"
                     "}}\n";
  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test");

  {
    FIND("test", 0, 2, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 0, -1);
  }
}

TEST_F(TestRootDefAnalyzer, TerminationP) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(const char* P1);\n"
                     "void API_2(const char* P1, int* P2);\n"
                     "void API_3(int P1);\n"
                     "void test() {\n"
                     "  API_1(\"Hello.txt\");\n"
                     "  const char *Var1 = \"World.txt\";\n"
                     "  API_1(Var1);\n"
                     "  char Var2[1024] = \"Hello.txt\";"
                     "  int Var3 = 0;\n"
                     "  API_2(Var2, &Var3);\n"
                     "  API_3(Var3);\n"
                     "}}";

  ASSERT_TRUE(loadCPP(CODE));

  auto DirectionReport = std::make_shared<DirectionAnalysisReport>();
  ASSERT_TRUE(DirectionReport);
  ASSERT_TRUE(addOutParam(*DirectionReport, SC->getLLVMModule(), "API_2", 1));

  RDExtension Extension;
  Extension.addTermination("API_1", 0);
  Extension.addTermination("API_2", 0);
  Extension.setDirectionReport(DirectionReport);

  BUILD(0, &Extension, "test");

  {
    // T1: Termination lets RootDefAnalyzer return a terminated state
    // as a root definition.
    // 1st argument of API_1("Hello.txt")
    //     -> RootDefinition should be 1st argument of API_1("Hello.txt")
    FIND("test", 0, 3, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 3, 0);
  }

  {
    // T2: Termination lets RootDefAnalyzer not to trace further.
    // 1st argument of API_1(Var1)
    //     -> RootDefinition should be 1st argument of API_1(Var1)
    FIND("test", 0, 7, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 7, 0);
  }

  {
    // T3: Termination lets RootDefAnalyzer not to trace further when
    // tracing target target is a changed target from output argument
    // 1st argument of API_3(Var3)
    //     -> Rootdefinition should be 1st argument of API_2(Var2)
#if LLVM_VERSION_MAJOR < 17
    FIND("test", 0, 16, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 14, 0);
#else
    FIND("test", 0, 15, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 13, 0);
#endif
  }
}

TEST_F(TestRootDefAnalyzer, TerminationN) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(const char* P1);\n"
                     "void test() {\n"
                     "  const char *Var1 = \"World.txt\";\n"
                     "  API_1(Var1);\n"
                     "}}";
  ASSERT_TRUE(loadCPP(CODE));

  RDExtension Extension;
  Extension.addTermination("API_2", 0);
  BUILD(0, &Extension, "test");

  {
    // T1: Should not stop if it is not termination
    // 1st argument of API_1(Var1)
    //     -> const char *Var1 = "World.txt";
    FIND("test", 0, 4, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 2, -1);
  }
}

TEST_F(TestRootDefAnalyzer, AliasP) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(int);\n"
                     "int EXT_1(int);\n"
                     "void sub(int *P1, int *P2) {\n"
                     "  *P2 = 30;\n"
                     "  *P1 = EXT_1(*P2);\n"
                     "}\n"
                     "void test() {\n"
                     "  int Var1 = 10, Var2 = 20;\n"
                     "  sub(&Var1, &Var2);\n"
                     "  API_1(Var1);\n"
                     "}}";
  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test");

  {
    // T1: *P1 = EXT_1(*P2) -> Target is changed from P1 to P2.
    //     *P2 = 30 should be found.
    FIND("test", 0, 8, 0);
    VERIFY_NUM(1);
#if LLVM_VERSION_MAJOR < 17
    VERIFY_INST("sub", 0, 7, -1);
#endif
  }
}

TEST_F(TestRootDefAnalyzer, AliasN) {
  const char *CODE = "extern \"C\" {\n"
                     "void API_1(int);\n"
                     "void test() {\n"
                     "  int Var1 = 1, Var2 = 2;\n"
                     "  int *Var3 = &Var2;\n"
                     "  *Var3 = 10;\n"
                     "  API_1(Var1);\n"
                     "  Var3 = &Var1;\n"
                     "}}\n";

  ASSERT_TRUE(loadCPP(CODE));
  BUILD(0, nullptr, "test");

  {
    // T1: API_1(Var1) -> int Var = 1;
    FIND("test", 0, 12, 0);
    VERIFY_NUM(1);
    VERIFY_INST("test", 0, 4, -1);
  }
}

#define RETURN_ERR(RET)                                                        \
  do {                                                                         \
    llvm::outs() << __LINE__;                                                  \
    return RET;                                                                \
  } while (0)

static inline bool verifyFirstUse(IRAccessHelper *IRAH,
                                  RootDefAnalyzer &Analyzer,
                                  std::string FuncName, unsigned BIdx,
                                  unsigned IIdx, unsigned AIdx,
                                  std::set<RDArgIndex> Answers,
                                  bool Debug = false) {

  auto *I = IRAH->getInstruction(FuncName, BIdx, IIdx);
  if (!I) {
    llvm::outs() << FuncName << " : " << BIdx << " : " << IIdx << "\n";
    RETURN_ERR(false);
  }
  if (Debug)
    llvm::outs() << "Instruction: " << *I << "\n";

  auto RDs = Analyzer.getRootDefinitions(I->getOperandUse(AIdx));
  if (RDs.size() != 1) {
    llvm::outs() << "RD (" << RDs.size() << ")\n";
    for (auto &RD : RDs)
      llvm::outs() << RD << "\n";
    RETURN_ERR(false);
  }
  if (Debug)
    llvm::outs() << "Root Definition: " << *RDs.begin() << "\n";

  auto &FUs = RDs.begin()->getFirstUses();
  if (FUs.size() != Answers.size()) {
    llvm::outs() << "Answers ======\n";
    for (auto &Answer : Answers)
      llvm::outs() << Answer << "\n";

    llvm::outs() << "Founds ======\n";
    for (auto &FU : FUs)
      llvm::outs() << FU << "\n";

    RETURN_ERR(false);
  }

  for (auto &Answer : Answers) {
    if (FUs.find(Answer) != FUs.end())
      continue;

    llvm::outs() << "Answers ======\n";
    for (auto &Answer : Answers)
      llvm::outs() << Answer << "\n";

    llvm::outs() << "Founds ======\n";
    for (auto &FU : FUs)
      llvm::outs() << FU << "\n";

    RETURN_ERR(false);
  }

  return true;
}

TEST_F(TestRootDefAnalyzer, FirstUseP) {

  const char *CODE =
      "extern \"C\" {\n"

      "void API_1(int P1);\n"
      "void API_2(int P1);\n"
      "void API_3(int P1);\n"
      "int API_4();\n"
      "void API_5(int *P1, int P2);\n"
      "class CLS {\n"
      "public:\n"
      "  CLS(int *P1, int P2) { API_5(P1, P2); };\n"
      "};\n"
      "int sub_1() {\n"
      "  int Var1 = 10;\n"
      "  API_1(Var1);\n"
      "  return Var1;\n"
      "}\n"

      "void test_directFU() {\n"
      "  API_1(10);\n" // First use should be itself.
      "}\n"

      "void test_indirectFU() {\n"
      "  int Var1 = 1;\n"
      "  int Var2 = 10;\n"
      "  API_1(Var1);\n" // First use should be itself.
      "}\n"

      "void test_unionFU() {\n"
      "  int Var1 = 1, Var2;\n"
      "  if (API_4() == 10) {\n"
      "    API_1(Var1);\n" // In this path, API_1 0th argument is First Use.
      "  } else if (API_4() == 20) {\n"
      "    API_2(Var1);\n" // In this path, API_2 0th argument is First Use.
      "  } else {\n"
      "    Var2 = 30;\n" // No First Use.
      "  }\n"
      "  API_3(Var1);\n" // Union: API_1 0th argument, API_2 0th argument, and
                         // itself because there is no first use in paths to
                         // definition.
      "}\n"

      "void test_subroutineFU() {\n"
      "  API_2(sub_1());\n"
      "}}\n"

      "void test_classUTFU() {\n"
      "  int Var1[] = { 0, 1, 2 };\n"
      "  CLS(Var1, 3);\n"
      "}\n";

  ASSERT_TRUE(loadCPP(CODE));

  {
    BUILD(0, nullptr, "test_directFU");

    std::set<RDArgIndex> Answers;
    auto *I = IRAH->getInstruction("test_directFU", 0, 0);
    ASSERT_TRUE(I);
    ASSERT_TRUE(isa<CallBase>(I));
    Answers.emplace(*dyn_cast<CallBase>(I), 0);
    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "test_directFU", 0, 0, 0,
                               Answers));
  }

  {
    BUILD(0, nullptr, "test_indirectFU");

    std::set<RDArgIndex> Answers;
    auto *I = IRAH->getInstruction("test_indirectFU", 0, 7);
    ASSERT_TRUE(I);
    ASSERT_TRUE(isa<CallBase>(I));
    Answers.emplace(*dyn_cast<CallBase>(I), 0);
    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "test_indirectFU", 0, 7,
                               0, Answers));
  }

  {
    BUILD(0, nullptr, "test_unionFU");

    std::set<RDArgIndex> Answers;

    auto *I1 = IRAH->getInstruction("test_unionFU", 1, 1);
    ASSERT_TRUE(I1);
    ASSERT_TRUE(isa<CallBase>(I1));
    Answers.emplace(*dyn_cast<CallBase>(I1), 0);

    auto *I2 = IRAH->getInstruction("test_unionFU", 3, 1);
    ASSERT_TRUE(I2);
    ASSERT_TRUE(isa<CallBase>(I2));
    Answers.emplace(*dyn_cast<CallBase>(I2), 0);

    auto *I3 = IRAH->getInstruction("test_unionFU", 6, 1);
    ASSERT_TRUE(I3);
    ASSERT_TRUE(isa<CallBase>(I3));
    Answers.emplace(*dyn_cast<CallBase>(I3), 0);

    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "test_unionFU", 6, 1, 0,
                               Answers));
  }

  {
    BUILD(0, nullptr, "test_subroutineFU");

    std::set<RDArgIndex> Answers;

    auto *I = IRAH->getInstruction("sub_1", 0, 4);
    ASSERT_TRUE(I);
    ASSERT_TRUE(isa<CallBase>(I));
    Answers.emplace(*dyn_cast<CallBase>(I), 0);
    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "test_subroutineFU", 0, 1,
                               0, Answers));
  }

  {
    BUILD(0, nullptr, "_Z14test_classUTFUv");

    std::set<RDArgIndex> Answers;
    auto *I = IRAH->getInstruction("_ZN3CLSC2EPii", 0, 12);
    ASSERT_TRUE(I);
    ASSERT_TRUE(isa<CallBase>(I));
    Answers.emplace(*dyn_cast<CallBase>(I), 0);
    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "_ZN3CLSC2EPii", 0, 12, 0,
                               Answers));

    Answers.clear();
    Answers.emplace(*dyn_cast<CallBase>(I), 1);
    ASSERT_TRUE(verifyFirstUse(IRAH.get(), *Analyzer, "_ZN3CLSC2EPii", 0, 12, 1,
                               Answers));
  }
}

TEST_F(TestRootDefAnalyzer, FirstUseN) {

  const char *CODE =
      "extern \"C\" {\n"

      "void API_1(int P1);\n"

      "void SUB_1(int P1) {}\n"
      "void SUB_2(int P1) {\n"
      "  API_1(P1);\n"
      "}\n"

      "void test_noFU_definedfunc() {\n"
      "  int Var1 = 10;\n"
      "  SUB_1(Var1);\n" // This function should be not considered as
                         // a first use because it is defined function.
      "}\n"

      "void test_noFU_prevFU() {\n"
      "  int Var1 = 10;\n" // This will be cached by tracing Var1
                           // from API_1(Var1)
      "  SUB_1(Var1);\n"   // SUB_1(Var1) should have no First Use even if
                           // Var1 = 10 that will be visited was cached.
      "  API_1(Var1);\n"
      "}\n"

      "void test_noFU_subroutine() {\n"
      "  int Var1 = 10;\n"
      "  SUB_2(Var1);\n" // Var1's first use is 0th argument of AP, however,
                         // RDAnalysis does not trace SUB_2 body because Var1
                         // is copied parameter that can not be a defiition.
      "  SUB_1(Var1);\n"
      "}}";
  ASSERT_TRUE(loadCPP(CODE));

  {
    BUILD(0, nullptr, "test_noFU_definedfunc");
    FIND("test_noFU_definedfunc", 0, 4, 0);
    ASSERT_EQ(RootDefs.size(), 1);

    auto &FUs = RootDefs.begin()->getFirstUses();
    ASSERT_EQ(FUs.size(), 0);
  }

  {
    BUILD(0, nullptr, "test_noFU_prevFU");

    auto *API_1 = IRAH->getInstruction("test_noFU_prevFU", 0, 6);
    ASSERT_TRUE(API_1);

    Analyzer->getRootDefinitions(API_1->getOperandUse(0));

    auto *SUB_1 = IRAH->getInstruction("test_noFU_prevFU", 0, 4);
    ASSERT_TRUE(SUB_1);

    auto RDs = Analyzer->getRootDefinitions(SUB_1->getOperandUse(0));
    ASSERT_EQ(RDs.size(), 1);

    auto &FUs = RDs.begin()->getFirstUses();
    ASSERT_EQ(FUs.size(), 0);
  }

  {
    BUILD(0, nullptr, "test_noFU_subroutine");

    auto *SUB_1 = IRAH->getInstruction("test_noFU_subroutine", 0, 6);
    ASSERT_TRUE(SUB_1);

    auto RDs = Analyzer->getRootDefinitions(SUB_1->getOperandUse(0));
    ASSERT_EQ(RDs.size(), 1);

    auto &FUs = RDs.begin()->getFirstUses();
    ASSERT_EQ(FUs.size(), 0);
  }
}

TEST_F(TestRootDefAnalyzer, ContinueFromCacheInLoopP) {

  const char *CODE = "void API(int *P);\n"
                     "int sub(int *P) { return *P; }\n"
                     "void test() {\n"
                     "  int Var1 = 10;\n"
                     "  int Var2 = 20;\n"
                     "  for (int S = 0; S < 10; ++S) {\n"
                     "    Var1 = sub(&Var1);\n"
                     "    Var2 = sub(&Var1);\n"
                     "  }\n"
                     "  API(&Var2);\n"
                     "}";

  ASSERT_TRUE(loadC(CODE));
  BUILD(0, nullptr, "test");

  FIND("test", 4, 0, 0);
  VERIFY_NUM(2);
  VERIFY_INST("test", 0, 4, -1);
  VERIFY_INST("test", 0, 6, -1);
}

TEST_F(TestRootDefAnalyzer, CyclicMemorySSA) {

  const char *CODE = "void API(int*);\n"
                     "void test() {\n"
                     "  int Var1 = 1;\n"
                     "  int Var2 = 0;\n"
                     "  while (Var2 < 100) {\n"
                     "    if (Var2 == 0) continue;\n"
                     "    Var1 *= Var2;\n"
                     "    Var2 += Var1;\n"
                     "  }\n"
                     "  API(&Var1);\n"
                     "}";

  ASSERT_TRUE(loadC(CODE));
  BUILD(0, nullptr, "test");

  FIND("test", 5, 0, 0);
  VERIFY_NUM(2);
  VERIFY_INST("test", 0, 3, -1);
  VERIFY_INST("test", 0, 5, -1);
}

/*
TEST_F(TestRootDefAnalyzer, ConstExprP) {
  const char* CODE =
    "extern \"C\" {\n"
    "char *GVar1 = nullptr;\n"
    "void API_1(void *P1);\n"
    "void test() {\n"
    "  API_1(&GVar1);\n"
    "}}\n";
  ASSERT_TRUE(loadCPP(CODE));
  BUILD("test");
  FIND("test", 0, 0, 0);
  VERIFY_NUM(1);
  VERIFY_GLOBAL("GVar1", -1);
}
*/
