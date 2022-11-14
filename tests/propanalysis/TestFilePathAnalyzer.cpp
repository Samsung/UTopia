#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/propanalysis/FilePathAnalyzer.h"

using namespace ftg;

class TestFilePathAnalyzer : public TestPropAnalyzer {

protected:
  std::unique_ptr<FilePathAnalyzer>
  analyze(IRAccessHelper &IRAccess, const std::vector<std::string> &FuncNames,
          const FilePathAnalysisReport *PreReport = nullptr) {
    std::vector<const llvm::Function *> Funcs;
    for (const auto &FuncName : FuncNames) {
      const auto *Func = IRAccess.getFunction(FuncName);
      if (!Func)
        continue;

      Funcs.push_back(Func);
    }

    IndCallSolverMgr Solver;
    return std::make_unique<FilePathAnalyzer>(&Solver, Funcs, PreReport);
  }

  bool checkTrue(const FilePathAnalysisReport &Report, IRAccessHelper &IRAccess,
                 std::string FuncName, unsigned ArgIndex,
                 std::vector<int> Indices = {}) {
    const auto *A = getArg(IRAccess, FuncName, ArgIndex);
    if (!A)
      return false;

    return Report.has(*A, Indices) && Report.get(*A, Indices);
  }
};

TEST_F(TestFilePathAnalyzer, AnalyzeP) {
  const std::string CODE1 = "#include <fcntl.h>\n"
                            "#include <stdio.h>\n"
                            "#include <unistd.h>\n"
                            "#include <string>\n"
                            "extern \"C\" {\n"
                            "void wrap(char *P1) {\n"
                            "  close(open(P1, O_RDONLY));\n"
                            "}\n"
                            "void test_fopen(char *P1) {\n"
                            "  fclose(fopen(P1, \"rb\"));\n"
                            "}\n"
                            "void test_open(char *P1) {\n"
                            "  close(open(P1, O_RDONLY));\n"
                            "}\n"
                            "void test_interprocedure(char *P1) {\n"
                            "  wrap(P1);\n"
                            "}\n"
                            "void test_string(char *P1) {\n"
                            "  std::string str(P1);\n"
                            "  close(open(str.c_str(), O_RDONLY));\n"
                            "}}";
  auto IRAccess1 =
      load(CODE1, CompileHelper::SourceType_CPP, "FilePathAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess1);

  std::vector<std::string> Funcs1 = {"test_fopen", "test_open",
                                     "test_interprocedure", "test_string"};
  auto FAnalyzer1 = analyze(*IRAccess1, Funcs1);
  ASSERT_TRUE(FAnalyzer1);

  const auto &Report1 = FAnalyzer1->report();
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_fopen", 0));
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_open", 0));
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_interprocedure", 0));
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_string", 0));
  ;
}
