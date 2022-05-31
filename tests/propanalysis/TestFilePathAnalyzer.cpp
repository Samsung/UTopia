#include "TestPropAnalyzer.hpp"
#include "ftg/indcallsolver/IndCallSolverImpl.h"
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

    return std::make_unique<FilePathAnalyzer>(
        std::make_shared<IndCallSolverImpl>(), Funcs, PreReport);
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
                            "#include <unistd.h>\n"
                            "extern \"C\" {\n"
                            "void wrap_default(char *P1) {\n"
                            "  close(open(P1, O_RDONLY));\n"
                            "}\n"
                            "void test_default(char *P1) {\n"
                            "  close(open(P1, O_RDONLY));\n"
                            "}\n"
                            "void test_interprocedure(char *P1) {\n"
                            "  wrap_default(P1);\n"
                            "}\n"
                            "}\n";
  auto IRAccess1 =
      load(CODE1, CompileHelper::SourceType_CPP, "FilePathAnalyzer", "-O0");
  ASSERT_TRUE(IRAccess1);

  std::vector<std::string> Funcs1 = {"test_default", "test_interprocedure"};
  auto FAnalyzer1 = analyze(*IRAccess1, Funcs1);
  ASSERT_TRUE(FAnalyzer1);

  const auto &Report1 = FAnalyzer1->report();
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_default", 0));
  ASSERT_TRUE(checkTrue(Report1, *IRAccess1, "test_interprocedure", 0));
  ;
}
