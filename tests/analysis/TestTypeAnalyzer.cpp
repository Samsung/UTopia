#include "TestHelper.h"
#include "ftg/analysis/TypeAnalyzer.h"
#include "ftg/utils/StringUtil.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;

class TestTypeAnalyzer : public ::testing::Test {
protected:
  std::unique_ptr<SourceCollection> SC;
  bool load(const std::string &BaseDir, const std::string &SrcPath) {
    std::vector<std::string> SrcPaths = {SrcPath};
    auto CH = TestHelperFactory().createCompileHelper(
        BaseDir, SrcPaths, "-O0 -g", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    return !!SC;
  }
};

TEST_F(TestTypeAnalyzer, getReportP) {
  std::string Code = "enum E1 { M1, M2, M3 };\n"
                     "namespace N1 { enum E2 { M1 = 3, M2, M3 }; }\n"
                     "namespace { enum E3 { M1 = 6, M2, M3 }; }\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));

  TypeAnalyzer Analyzer(SC->getASTUnits());
  auto Report = Analyzer.getReport();
  ASSERT_TRUE(Report);

  Json::Value AnswerJson = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": {
            "(anonymous namespace)::E3": {
              "Enumerators": [
                {"Name": "M1","Value": 6},
                {"Name": "M2","Value": 7},
                {"Name": "M3","Value": 8}
              ],
              "Name": "(anonymous namespace)::E3"
            },
            "E1": {
              "Enumerators": [
                { "Name": "M1", "Value": 0 },
                { "Name": "M2", "Value": 1 },
                { "Name": "M3", "Value": 2 }
              ],
              "Name": "E1"
            },
            "N1::E2": {
              "Enumerators": [
                { "Name": "M1", "Value": 3 },
                { "Name": "M2", "Value": 4 },
                { "Name": "M3", "Value": 5 }
              ],
              "Name": "N1::E2"
            }
          }
        }
      }
  )");
  ASSERT_EQ(Report->toJson(), AnswerJson);
}

TEST_F(TestTypeAnalyzer, getReportN) {
  std::string Code = "";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));

  TypeAnalyzer Analyzer(SC->getASTUnits());
  auto Report = Analyzer.getReport();
  ASSERT_TRUE(Report);

  Json::Value AnswerJson = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": null
        }
      }
  )");
  ASSERT_EQ(Report->toJson(), AnswerJson);
}