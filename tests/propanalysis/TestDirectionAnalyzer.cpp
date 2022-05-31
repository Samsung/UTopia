#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverImpl.h"
#include "ftg/propanalysis/DirectionAnalyzer.h"

using namespace ftg;

class TestDirectionAnalyzer : public TestPropAnalyzer {

protected:
  std::unique_ptr<DirectionAnalyzer>
  analyze(IRAccessHelper &IRAccess, const std::vector<std::string> &FuncNames,
          const DirectionAnalysisReport *PreReport = nullptr) {
    std::vector<const llvm::Function *> Funcs;
    for (const auto &FuncName : FuncNames) {
      const auto *Func = IRAccess.getFunction(FuncName);
      if (!Func)
        continue;

      Funcs.push_back(Func);
    }

    return std::make_unique<DirectionAnalyzer>(
        std::make_shared<IndCallSolverImpl>(), Funcs, PreReport);
  }

  bool checkTrue(const DirectionAnalysisReport &Report,
                 IRAccessHelper &IRAccess, std::string FuncName,
                 unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices) == Dir_Out;
  }

  bool checkFalse(const DirectionAnalysisReport &Report,
                  IRAccessHelper &IRAccess, std::string FuncName,
                  unsigned ArgIndex, std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices) != Dir_Out;
  }
};

TEST_F(TestDirectionAnalyzer, AnalyzeP) {
  const std::string CODE1 = "#include <string.h>\n"
                            "extern \"C\" {\n"
                            "struct ST1 { int *F1; };\n"
                            "void test_intrinsic_memset(void *P1) {\n"
                            "  memset(P1, 0, 0);\n"
                            "}\n"
                            "void test_intrinsic_memcpy(void *P1) {\n"
                            "  memcpy(P1, \"Str1\", 4);\n"
                            "}\n"
                            "void test_output(int *P1, int *P2, int P3) {\n"
                            "  *P1 = 10;\n"
                            "  *P2 = P3;\n"
                            "}\n"
                            "void test_field(struct ST1 *P1) {\n"
                            "  *P1->F1 = 10;\n"
                            "}\n"
                            "}";
  auto IRAccess1 =
      load(CODE1, CompileHelper::SourceType_CPP, "DirectionAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess1);

  std::vector<std::string> Funcs1 = {"test_intrinsic_memset",
                                     "test_intrinsic_memcpy", "test_output",
                                     "test_field"};
  auto DAnalyzer1 = analyze(*IRAccess1, Funcs1);
  ASSERT_TRUE(DAnalyzer1);

  ASSERT_TRUE(
      checkTrue(DAnalyzer1->result(), *IRAccess1, "test_intrinsic_memset", 0));
  ASSERT_TRUE(
      checkTrue(DAnalyzer1->result(), *IRAccess1, "test_intrinsic_memcpy", 0));
  ASSERT_TRUE(checkTrue(DAnalyzer1->result(), *IRAccess1, "test_output", 0));
  ASSERT_TRUE(checkTrue(DAnalyzer1->result(), *IRAccess1, "test_output", 1));
  ASSERT_TRUE(
      checkTrue(DAnalyzer1->result(), *IRAccess1, "test_field", 0, {0}));

  const std::string CODE2 = "#include <string.h>\n"
                            "extern \"C\" {\n"
                            "struct ST1 { int *F1; };\n"
                            "void test_output(int *, int *, int);\n"
                            "void test_field(struct ST1 *P1);\n"
                            "void test_reuse(int *P1, struct ST1 *P2) {\n"
                            "  test_output(P1, nullptr, 0);\n"
                            "  test_field(P2);\n"
                            "}\n"
                            "}";
  auto IRAccess2 =
      load(CODE2, CompileHelper::SourceType_CPP, "DirectionAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess2);

  std::vector<std::string> Funcs2 = {"test_reuse"};
  auto DAnalyzer2 = analyze(*IRAccess2, Funcs2, &DAnalyzer1->result());
  ASSERT_TRUE(DAnalyzer2);

  ASSERT_TRUE(checkTrue(DAnalyzer2->result(), *IRAccess2, "test_reuse", 0));
  ASSERT_TRUE(
      checkTrue(DAnalyzer2->result(), *IRAccess2, "test_reuse", 1, {0}));
}

TEST_F(TestDirectionAnalyzer, AnalyzeN) {
  const std::string CODE = "extern \"C\" {\n"
                           "void test_nonout(int *P1, int P2) {\n"
                           "  *P1 = P2;\n"
                           "}\n"
                           "}";
  auto IRAccess =
      load(CODE, CompileHelper::SourceType_CPP, "DirectionAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess);

  std::vector<std::string> Funcs = {"test_nonout"};
  auto DAnalyzer = analyze(*IRAccess, Funcs);
  ASSERT_TRUE(DAnalyzer);

  ASSERT_TRUE(checkFalse(DAnalyzer->result(), *IRAccess, "test_nonout", 1));
}
