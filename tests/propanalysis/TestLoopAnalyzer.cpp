#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/propanalysis/LoopAnalyzer.h"

namespace ftg {

class TestLoopAnalyzer : public TestPropAnalyzer {

protected:
  std::unique_ptr<LoopAnalyzer>
  analyze(IRAccessHelper &IRAccess, const std::vector<std::string> &FuncNames,
          const LoopAnalysisReport *PreReport = nullptr) {
    std::vector<const llvm::Function *> Funcs;
    for (const auto &FuncName : FuncNames) {
      const auto *Func = IRAccess.getFunction(FuncName);
      if (!Func)
        continue;

      Funcs.push_back(Func);
    }

    IndCallSolverMgr Solver;
    return std::make_unique<LoopAnalyzer>(&Solver, Funcs, FAM, PreReport);
  }

  bool checkTrue(bool AnswerLoopExit, unsigned AnswerLoopDepth,
                 const LoopAnalysisReport &Report, IRAccessHelper &IRAccess,
                 std::string FuncName, unsigned ArgIndex,
                 std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    if (!Report.has(*A, Indices))
      return false;

    auto Result = Report.get(*A, Indices);
    return Result.LoopExit == AnswerLoopExit &&
           Result.LoopDepth == AnswerLoopDepth;
  }

  bool checkFalse(const LoopAnalysisReport &Report, IRAccessHelper &IRAccess,
                  std::string FuncName, unsigned ArgIndex,
                  std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    if (!Report.has(*A, Indices))
      return false;

    return !Report.get(*A, Indices).LoopExit;
  }
};

TEST_F(TestLoopAnalyzer, AnalyzeP) {
  const std::string CODE = "extern \"C\" {\n"
                           "void EXTERNAL();\n"
                           "void test_exitcond(int P) {\n"
                           "  for (int i = 0; i < P; ++i) {\n"
                           "    EXTERNAL();\n"
                           "  }\n"
                           "}\n"
                           "void test_nested(int P) {\n"
                           "  for (int i = 0; i < 10; ++i) {\n"
                           "    for (int j = 0; j < P; ++j) {\n"
                           "      EXTERNAL();\n"
                           "    }\n"
                           "  }\n"
                           "}\n"
                           "void test_exitcond_rel1(int P) {\n"
                           "  int J = P + 10;\n"
                           "  for (int i = 0; i < J; ++i) {\n"
                           "    EXTERNAL();\n"
                           "  }\n"
                           "}\n"
#if LLVM_VERSION_MAJOR >= 12
                           "void test_exitcond_rel2(int P) {\n"
                           "  for (int i = 0; i < P + 10; ++i) {\n"
                           "    EXTERNAL();\n"
                           "  }\n"
                           "}\n"
#endif
                           "}";

  auto IRAccess =
      load(CODE, CompileHelper::SourceType_CPP, "PropAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess);

  std::vector<std::string> Funcs = {
    "test_exitcond",
    "test_nested",
    "test_exitcond_rel1",
#if LLVM_VERSION_MAJOR >= 12
    "test_exitcond_rel2"
#endif
  };
  auto LAnalyzer = analyze(*IRAccess, Funcs);
  ASSERT_TRUE(LAnalyzer);
#if LLVM_VERSION_MAJOR < 17
  ASSERT_TRUE(
      checkTrue(true, 1, LAnalyzer->result(), *IRAccess, "test_exitcond", 0));
  ASSERT_TRUE(
      checkTrue(true, 2, LAnalyzer->result(), *IRAccess, "test_nested", 0));
  ASSERT_TRUE(checkTrue(true, 1, LAnalyzer->result(), *IRAccess,
                        "test_exitcond_rel1", 0));
#if LLVM_VERSION_MAJOR >= 12
  ASSERT_TRUE(checkTrue(true, 1, LAnalyzer->result(), *IRAccess,
                        "test_exitcond_rel2", 0));
#endif
#endif
}

TEST_F(TestLoopAnalyzer, AnalyzeN) {
  const std::string CODE = "extern \"C\" {\n"
                           "void EXTERNAL();\n"
                           "void test_nonexit(int P) {\n"
                           "  for (int i = P; i < 10; ++i) {\n"
                           "    EXTERNAL();\n"
                           "  }\n"
                           "}\n"
#if LLVM_VERSION_MAJOR < 12
                           "void test_exitcond_rel(int P) {\n"
                           "  for (int i = 0; i < P + 10; ++i) {\n"
                           "    EXTERNAL();\n"
                           "  }\n"
                           "}\n"
#endif
                           "}";
  auto IRAccess =
      load(CODE, CompileHelper::SourceType_CPP, "PropAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess);

  std::vector<std::string> Funcs = {
    "test_nonexit",
#if LLVM_VERSION_MAJOR < 12
    "test_exitcond_rel"
#endif
  };
  auto LAnalyzer = analyze(*IRAccess, Funcs);
  ASSERT_TRUE(LAnalyzer);
  ASSERT_TRUE(checkFalse(LAnalyzer->result(), *IRAccess, "test_nonexit", 0));
#if LLVM_VERSION_MAJOR < 12
  ASSERT_TRUE(
      checkFalse(LAnalyzer->result(), *IRAccess, "test_exitcond_rel", 0));
#endif
}

TEST_F(TestLoopAnalyzer, SerializeP) {
  LoopAnalysisReport Report1;
  Report1.set("test(0)", {true, 1});
  LoopAnalysisReport Report2;
  ASSERT_TRUE(Report2.fromJson(Report1.toJson()));
  ASSERT_TRUE(Report2.has("test(0)"));
  auto Result = Report2.get("test(0)");
  ASSERT_EQ(Result.LoopExit, true);
  ASSERT_EQ(Result.LoopDepth, 1);
}

TEST_F(TestLoopAnalyzer, SerializeN) {
  LoopAnalysisReport Report;
  ASSERT_FALSE(Report.fromJson(""));
}

} // namespace ftg
