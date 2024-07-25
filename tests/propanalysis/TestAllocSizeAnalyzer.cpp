#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/propanalysis/AllocSizeAnalyzer.h"

namespace ftg {

class TestAllocSizeAnalyzer : public TestPropAnalyzer {

protected:
  std::unique_ptr<AllocSizeAnalyzer>
  analyze(IRAccessHelper &IRAccess, const std::vector<std::string> &FuncNames,
          const AllocSizeAnalysisReport *PreReport = nullptr) {
    std::vector<const llvm::Function *> Funcs;
    for (const auto &FuncName : FuncNames) {
      const auto *Func = IRAccess.getFunction(FuncName);
      if (!Func)
        continue;

      Funcs.push_back(Func);
    }

    IndCallSolverMgr Solver;
    return std::make_unique<AllocSizeAnalyzer>(&Solver, Funcs, PreReport);
  }

  bool checkTrue(bool Answer, const AllocSizeAnalysisReport &Report,
                 IRAccessHelper &IRAccess, std::string FuncName,
                 unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices) == Answer;
  }

  bool checkFalse(bool Answer, const AllocSizeAnalysisReport &Report,
                  IRAccessHelper &IRAccess, std::string FuncName,
                  unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices) != Answer;
  }

  bool checkNone(const AllocSizeAnalysisReport &Report,
                 IRAccessHelper &IRAccess, std::string FuncName,
                 unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return !Report.has(*A, Indices);
  }
};

TEST_F(TestAllocSizeAnalyzer, AnalyzeP) {
  const std::string CODE1 =
      "#include <stdlib.h>\n"
      "#include <string>\n"
      "#include <vector>\n"
      "extern \"C\" {\n"
      "void extern_func(int *Dst, int *Src, int Len);\n"
      "void extern_func2(int *Dst, int *Src, int Len);\n"
      "struct teststruct{\n"
      "    int structint;\n"
      "    int* structintpoint;\n"
      "    char dummy[16];\n"
      "};\n"
      "void func1(int *dst, int *src, int len) {\n"
      "  malloc(len);\n"
      "}\n"
      "void func2(int *dst, int *src, int len) {\n"
      "  int len2 = len;\n"
      "  func1(dst, src, len2);\n"
      "}\n"
      "void func3(int *Dst, int *Src, int Len) {\n"
      "  func1(Dst, Src, *Dst);\n"
      "  int len_from_Src = *Src;\n"
      "  func2(Dst, Src, len_from_Src);\n"
      "}\n"
      "void func4(struct teststruct* A, struct teststruct B,\n"
      "           struct teststruct C) {\n"
      "  int *Dst, *Src;\n"
      "  struct teststruct D;\n"
      "  D.structint = B.structint;\n"
      "  D.structintpoint = B.structintpoint;\n"
      "  int len1 = A->structint;\n"
      "  int dummy = D.structint;\n"
      "  int* len2 = B.structintpoint;\n"
      "  int* dummy2 = A->structintpoint;\n"
      "  func1(Dst, Src, len1);\n"
      "  func1(Dst, Src, *len2);\n"
      "}\n"
      "void* check_alloc(void* ptr){\n"
      "  return ptr;\n"
      "}\n"
      "void* func5(struct teststruct* A, int len, void** pnt) {\n"
      " A->structint = 20;\n"
      " A->structintpoint = (int*) malloc(5);\n"
      " *pnt = malloc(10);\n"
      " return check_alloc(malloc(len));\n"
      "}\n"
      "void func_new(int P) {\n"
      "  int *Var = new int[P];\n"
      "  free(Var);\n"
      "}\n"
      "void func_string(int P0, int P1, int P2, int P3) {\n"
      "  char str[] = \"Hello\";\n"
      "  std::string message0(str, P0);\n"
      "  std::string message1(P1, 'a');\n"
      "  std::string message2(message1, P2, P3);\n"
      "}\n"
      "void func_vector(int P) {\n"
      "  std::vector<char> message(P);\n"
      "}\n"
      "void test_struct(struct teststruct *P1) {\n"
      "  free(malloc(P1->structint));\n"
      "}\n"
      "void test_calloc(int P1, int P2) {\n"
      "  void *Ptr = calloc(P1, P2);\n"
      "  if (Ptr) free(Ptr);\n"
      "}\n"
      "void test_realloc(int P1) {\n"
      "  void *Ptr = malloc(10);\n"
      "  if (!Ptr) return;\n"
      "  Ptr = realloc(Ptr, P1);\n"
      "  if (!Ptr) return;\n"
      "  free(Ptr);"
      "}"
      "}";
  auto IRAccess1 =
      load(CODE1, CompileHelper::SourceType_CPP, "AllocSizeAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess1);

  std::vector<std::string> Funcs1 = {
      "func1",       "func2",       "func3",       "func4",
      "func5",       "func_new",    "func_string", "func_vector",
      "test_struct", "test_calloc", "test_realloc"};
  auto AAnalyzer1 = analyze(*IRAccess1, Funcs1);
  ASSERT_TRUE(AAnalyzer1);

  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func1", 2));
  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func2", 2));
  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func3", 0));
  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func3", 1));
  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func5", 1));
  EXPECT_TRUE(checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func_new", 0));
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func_string", 0));
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func_string", 1));
#endif
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func_string", 3));
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func_vector", 0));
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "test_struct", 0, {0}));
#endif
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "test_calloc", 0));
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "test_calloc", 1));
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "test_realloc", 0));
#ifndef __arm__
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func4", 0, {0}));
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer1->result(), *IRAccess1, "func4", 1, {1}));
#endif
#endif

  const std::string CODE2 =
      "extern \"C\" {\n"
      "struct teststruct { int F1; int *F2; char F3[16]; };\n"
      "void test_struct(struct teststruct *P1);\n"
      "void func_new(int);\n"
      "void test_reuse(int P1, teststruct *P2) {\n"
      "  func_new(P1);\n"
      "  test_struct(P2);\n"
      "}\n"
      "}";
  auto IRAccess2 =
      load(CODE2, CompileHelper::SourceType_CPP, "AllocSizeAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess2);

  std::vector<std::string> Funcs2 = {"test_reuse"};
  auto AAnalyzer2 = analyze(*IRAccess2, Funcs2, &AAnalyzer1->result());
  ASSERT_TRUE(AAnalyzer2);

  EXPECT_TRUE(
      checkTrue(true, AAnalyzer2->result(), *IRAccess2, "test_reuse", 0));
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkTrue(true, AAnalyzer2->result(), *IRAccess2, "test_reuse", 1, {0}));
#endif
}

TEST_F(TestAllocSizeAnalyzer, AnalyzeN) {
  const std::string CODE =
      "#include <stdlib.h>\n"
      "#include <string>\n"
      "#include <vector>\n"
      "extern \"C\" {\n"
      "void extern_func(int *Dst, int *Src, int Len);\n"
      "void extern_func2(int *Dst, int *Src, int Len);\n"
      "struct teststruct{\n"
      "    int structint;\n"
      "    int* structintpoint;\n"
      "    char dummy[16];\n"
      "};\n"
      "void func1(int *dst, int *src, int len) {\n"
      "  malloc(len);\n"
      "}\n"
      "void func2(int *dst, int *src, int len) {\n"
      "  int len2 = len;\n"
      "  func1(dst, src, len2);\n"
      "}\n"
      "void func3(int *Dst, int *Src, int Len) {\n"
      "  func1(Dst, Src, *Dst);\n"
      "  int len_from_Src = *Src;\n"
      "  func2(Dst, Src, len_from_Src);\n"
      "}\n"
      "void func4(struct teststruct* A, struct teststruct B,\n"
      "           struct teststruct C) {\n"
      "  int *Dst, *Src;\n"
      "  struct teststruct D;\n"
      "  D.structint = B.structint;\n"
      "  D.structintpoint = B.structintpoint;\n"
      "  int len1 = A->structint;\n"
      "  int dummy = D.structint;\n"
      "  int* len2 = B.structintpoint;\n"
      "  int* dummy2 = A->structintpoint;\n"
      "  func1(Dst, Src, len1);\n"
      "  func1(Dst, Src, *len2);\n"
      "}\n"
      "void* check_alloc(void* ptr){\n"
      "  return ptr;\n"
      "}\n"
      "void* func5(struct teststruct* A, int len, void** pnt) {\n"
      " A->structint = 20;\n"
      " A->structintpoint = (int*) malloc(5);\n"
      " *pnt = malloc(10);\n"
      " return check_alloc(malloc(len));\n"
      "}\n"
      "void func_string(int P0, int P1, int P2, int P3) {\n"
      "  char str[] = \"Hello\";\n"
      "  std::string message0(str, P0);\n"
      "  std::string message1(P1, 'a');\n"
      "  std::string message2(message1, P2, P3);\n"
      "}\n"
      "void test_struct(struct teststruct *P1) {\n"
      "  free(malloc(P1->structint));\n"
      "}\n"
      "}";
  auto IRAccess =
      load(CODE, CompileHelper::SourceType_CPP, "AllocSizeAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess);

  std::vector<std::string> Funcs = {"func1",      "func2", "func3",
                                    "func4",      "func5", "func_string",
                                    "test_struct"};
  auto AAnalyzer = analyze(*IRAccess, Funcs);
  ASSERT_TRUE(AAnalyzer);

  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func1", 0));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func1", 1));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func2", 0));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func2", 1));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func3", 2));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func4", 2));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func5", 0));
  EXPECT_TRUE(checkTrue(false, AAnalyzer->result(), *IRAccess, "func5", 2));
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkFalse(true, AAnalyzer->result(), *IRAccess, "func_string", 2));
#endif
#ifndef __arm__
#if LLVM_VERSION_MAJOR < 17
  EXPECT_TRUE(
      checkTrue(false, AAnalyzer->result(), *IRAccess, "func4", 0, {1}));
  EXPECT_TRUE(
      checkTrue(false, AAnalyzer->result(), *IRAccess, "func4", 1, {0}));
#endif
  EXPECT_TRUE(checkNone(AAnalyzer->result(), *IRAccess, "test_struct", 0, {1}));
  EXPECT_TRUE(checkNone(AAnalyzer->result(), *IRAccess, "test_struct", 0, {2}));
#endif
}

} // namespace ftg
