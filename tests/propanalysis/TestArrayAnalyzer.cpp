#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/propanalysis/ArrayAnalyzer.h"

namespace ftg {

class TestArrayAnalyzer : public TestPropAnalyzer {

protected:
  std::unique_ptr<ArrayAnalyzer>
  analyze(IRAccessHelper &IRAccess, const std::vector<std::string> &FuncNames,
          const ArrayAnalysisReport *PreReport = nullptr) {
    std::vector<const llvm::Function *> Funcs;
    for (const auto &FuncName : FuncNames) {
      const auto *Func = IRAccess.getFunction(FuncName);
      if (!Func)
        continue;

      Funcs.push_back(Func);
    }

    IndCallSolverMgr Solver;
    return std::make_unique<ArrayAnalyzer>(&Solver, Funcs, FAM, PreReport);
  }

  bool checkTrue(int Answer, const ArrayAnalysisReport &Report,
                 IRAccessHelper &IRAccess, std::string FuncName,
                 unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices) == Answer;
  }
};

TEST_F(TestArrayAnalyzer, AnalyzeP) {
  const std::string CODE1 =
      "extern \"C\" {\n"
      "#include <string.h>\n"
      "struct ST1 { int *F1; int F2; };\n"
      "void test_default(int *Arr, int Len) {\n"
      "  memset(Arr, 0, Len);\n"
      "}\n"
      "void test_arraysubscript(int *Arr, int *Ptr, int *Unknown) {\n"
      "  Arr[0] = 1;\n"
      "  Ptr[1] = 1;\n"
      "}\n"
      "void test_loop(int *Arr, int Len) {\n"
      "  for (int S = 0; S < Len; S++) Arr[S] = 1;\n"
      "}\n"
      "void test_struct(struct ST1 *P1) {\n"
      "  test_loop(P1->F1, P1->F2);\n"
      "}\n"
      "}";

  auto IRAccess1 =
      load(CODE1, CompileHelper::SourceType_CPP, "ArrayAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess1);

  std::vector<std::string> Funcs1 = {"test_default", "test_arraysubscript",
                                     "test_loop", "test_struct"};
  auto AAnalyzer1 = analyze(*IRAccess1, Funcs1);
  ASSERT_TRUE(AAnalyzer1);

  ASSERT_TRUE(
      checkTrue(1, AAnalyzer1->result(), *IRAccess1, "test_default", 0));
  ASSERT_TRUE(checkTrue(ArrayAnalysisReport::ARRAY_NOLEN, AAnalyzer1->result(),
                        *IRAccess1, "test_arraysubscript", 1));
  ASSERT_TRUE(checkTrue(1, AAnalyzer1->result(), *IRAccess1, "test_loop", 0));
  ASSERT_TRUE(
      checkTrue(1, AAnalyzer1->result(), *IRAccess1, "test_struct", 0, {0}));

  const std::string CODE2 =
      "extern \"C\" {\n"
      "struct ST1 { int *F1; int F2; };\n"
      "void test_loop(int *Arr, int Len);\n"
      "void test_struct(struct ST1 *P1);\n"
      "void test_reuse(int *P1, int P2, struct ST1 *P3) {\n"
      "  test_loop(P1, P2);\n"
      "  test_struct(P3);\n"
      "}\n"
      "}\n";

  auto IRAccess2 =
      load(CODE2, CompileHelper::SourceType_CPP, "ArrayAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess2);

  std::vector<std::string> Funcs2 = {"test_reuse"};
  auto AAnalyzer2 = analyze(*IRAccess2, Funcs2, &AAnalyzer1->result());
  ASSERT_TRUE(AAnalyzer2);

  ASSERT_TRUE(checkTrue(1, AAnalyzer2->result(), *IRAccess2, "test_reuse", 0));
  ASSERT_TRUE(
      checkTrue(1, AAnalyzer2->result(), *IRAccess2, "test_reuse", 2, {0}));
}

TEST_F(TestArrayAnalyzer, AnalyzeN) {
  const std::string CODE = "extern \"C\" {\n"
                           "void extern_func(int *P);\n"
                           "void func1(int *Arr, int *Ptr, int *Unknown) {\n"
                           "  Arr[1] = 1;\n"
                           "  Ptr[0] = 1;\n"
                           "  extern_func(Unknown);\n"
                           "}\n"
                           "void func2(int *Arr, int Len) {\n"
                           "  for (int S = 0; S < Len; S++) Arr[S] = 1;\n"
                           "}}\n";

  auto IRAccess =
      load(CODE, CompileHelper::SourceType_CPP, "ArrayAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess);

  std::vector<std::string> Funcs = {"func1", "func2"};
  auto AAnalyzer = analyze(*IRAccess, Funcs);
  ASSERT_TRUE(AAnalyzer);

  ASSERT_TRUE(checkTrue(ArrayAnalysisReport::NO_ARRAY, AAnalyzer->result(),
                        *IRAccess, "func1", 1));
  ASSERT_TRUE(checkTrue(ArrayAnalysisReport::NO_ARRAY, AAnalyzer->result(),
                        *IRAccess, "func1", 2));
  ASSERT_TRUE(checkTrue(ArrayAnalysisReport::NO_ARRAY, AAnalyzer->result(),
                        *IRAccess, "func2", 1));
}

} // namespace ftg
