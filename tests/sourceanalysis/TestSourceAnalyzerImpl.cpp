#include "TestHelper.h"
#include "ftg/sourceanalysis/SourceAnalyzerImpl.h"
#include "testutil/APIManualLoader.h"
#include <gtest/gtest.h>

using namespace ftg;

class TestSourceImplAnalyzer : public TestBase {

protected:
  std::unique_ptr<SourceCollection> SC;
  std::unique_ptr<SourceAnalyzerImpl> Analyzer;

  bool analyze() {
    SC = CH->load();
    if (!SC)
      return false;

    Analyzer = std::make_unique<SourceAnalyzerImpl>(*SC);
    if (!Analyzer)
      return false;

    return true;
  }

  bool checkNonFile(const std::string &SourceName) {
    if (!Analyzer)
      return false;

    const auto &Report = Analyzer->getActualReport();

    if (Report.getEndOffset(SourceName) != 0)
      return false;

    if (Report.getIncludedHeaders(SourceName).size() != 0)
      return false;

    return true;
  }

  bool checkIncludedHeaders(const std::string &SourceName,
                            std::vector<std::string> Answer) {
    if (!Analyzer)
      return false;

    for (auto IncludedHeader :
         Analyzer->getActualReport().getIncludedHeaders(SourceName)) {
      auto Iter = std::find(Answer.begin(), Answer.end(), IncludedHeader);
      if (Iter == Answer.end())
        return false;
      Answer.erase(Iter);
    }

    if (Answer.size() != 0)
      return false;

    return true;
  }

  bool checkEndOffset(const std::string &SourceName, unsigned EndOffset) {
    if (!Analyzer)
      return false;

    return EndOffset == Analyzer->getActualReport().getEndOffset(SourceName);
  }
};

TEST_F(TestSourceImplAnalyzer, UnknownFileN) {
  const std::string CODE = "#include <iostream>";
  std::vector<std::string> Answers;

  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(checkNonFile("unknown.cpp"));
}

TEST_F(TestSourceImplAnalyzer, HeaderP) {
  const std::string CODE = "#include \"stdio.h\"\n"
                           "#include <assert.h>\n"
                           "#include <sys/acct.h>\n"
                           "#include \"sys/stat.h\"\n"
                           "#include <stdlib.h> // comment\n"
                           "#include \"memory.h\" // comment\n"
                           "#include <sys/bitypes.h> // comment\n"
                           "#include \"sys/file.h\" // comment\n";

  std::vector<std::string> Answers = {
      "\"stdio.h\"", "<assert.h>",   "<sys/acct.h>",    "\"sys/stat.h\"",
      "<stdlib.h>",  "\"memory.h\"", "<sys/bitypes.h>", "\"sys/file.h\""};

  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(checkIncludedHeaders("/tmp/sourceimplanalyzer.cpp", Answers));
}

TEST_F(TestSourceImplAnalyzer, HeaderN) {
  const std::string CODE = "void test();";
  std::vector<std::string> Answers;

  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(checkIncludedHeaders("/tmp/sourceimplanalyzer.cpp", Answers));
}

TEST_F(TestSourceImplAnalyzer, EndOffsetP) {
  const std::string CODE = "void test();\n";
  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(checkEndOffset("/tmp/sourceimplanalyzer.cpp", 13));
}

TEST_F(TestSourceImplAnalyzer, EndOffsetN) {
  const std::string CODE = "";
  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(checkEndOffset("/tmp/sourceimplanalyzer.cpp", 0));
}

TEST_F(TestSourceImplAnalyzer, SerializeP) {
  const std::string CODE = "#include <iostream>";
  ASSERT_TRUE(load(CODE, "sourceimplanalyzer", "-O0 -g",
                   CompileHelper::SourceType_CPP));
  ASSERT_TRUE(analyze());
  ASSERT_TRUE(Analyzer);

  const auto &Report = Analyzer->getActualReport();
  SourceAnalysisReport ClonedReport;
  ClonedReport.fromJson(Report.toJson());

  ASSERT_TRUE(Report.getEndOffset("/tmp/sourceimplanalyzer.cpp") ==
              ClonedReport.getEndOffset("/tmp/sourceimplanalyzer.cpp"));
  ASSERT_TRUE(Report.getIncludedHeaders("/tmp/sourceimplanalyzer.cpp") ==
              ClonedReport.getIncludedHeaders("/tmp/sourceimplanalyzer.cpp"));
  ASSERT_TRUE(Report.getSrcBaseDir() == ClonedReport.getSrcBaseDir());
}
