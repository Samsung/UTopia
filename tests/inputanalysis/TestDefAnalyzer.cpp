#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/inputanalysis/DefAnalyzer.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"
#include "testutil/APIManualLoader.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;
using namespace llvm;

class TestDefAnalyzer : public ::testing::Test {
protected:
  struct AnswerT {
    unsigned Offset;
    unsigned Length;
  };
  std::unique_ptr<UTLoader> Loader;

  bool check(const DefAnalyzer &Analyzer,
             const std::vector<AnswerT> &Answers) const {
    const auto &DefMap = Analyzer.get().getDefMap();
    if (DefMap.size() != Answers.size())
      return false;

    for (const auto &Answer : Answers) {
      auto It = std::find_if(DefMap.begin(), DefMap.end(),
                             [&Answer](const auto &It) -> bool {
                               if (!It.second)
                                 return false;

                               return Answer.Offset == It.second->Offset &&
                                      Answer.Length == It.second->Length;
                             });
      if (It == DefMap.end())
        return false;
    }
    return true;
  }
  std::unique_ptr<DefAnalyzer> load(std::string BaseDirPath,
                                    std::string CodePath,
                                    std::string CompileOptions,
                                    std::set<std::string> APINames) {
    std::vector<std::string> CodePaths = {CodePath};
    auto CH = TestHelperFactory().createCompileHelper(
        BaseDirPath, CodePaths, CompileOptions, CompileHelper::SourceType_CPP);
    if (!CH)
      return nullptr;

    auto AL = std::make_shared<APIManualLoader>(APINames);
    if (!AL)
      return nullptr;

    Loader = std::make_unique<UTLoader>(CH, AL);
    if (!Loader)
      return nullptr;

    auto Factory = createTCAnalyzerFactory("gtest");
    if (!Factory)
      return nullptr;

    auto Extractor = Factory->createTCExtractor(Loader->getSourceCollection());
    if (!Extractor)
      return nullptr;

    Extractor->load();
    std::vector<Unittest> TCs;
    for (const auto &TCName : Extractor->getTCNames()) {
      const auto &UT = Extractor->getTC(TCName);
      if (!UT)
        continue;

      TCs.emplace_back(*UT);
    }

    const auto &M = Loader->getSourceCollection().getLLVMModule();
    std::set<Function *> Funcs;
    for (const auto &APIName : APINames) {
      const auto *F = M.getFunction(APIName);
      if (!F)
        continue;

      Funcs.insert(const_cast<Function *>(F));
    }

    return std::make_unique<DefAnalyzer>(
        Loader.get(), TCs, Funcs,
        std::make_shared<DebugInfoMap>(Loader->getSourceCollection()));
  }
};

TEST_F(TestDefAnalyzer, StringP) {
  const std::string Code = R"(
  #include <gtest/gtest.h>
  extern "C" {
    void API(std::string);
    TEST(Test, Test) {
        API("a");
    }
  }
  )";
  std::vector<AnswerT> Answers = {{105, 3}};

  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::set<std::string> APINames = {"API"};
  auto Analyzer = load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp"),
                       "-O0 -g -w", APINames);
  ASSERT_TRUE(Analyzer);
  ASSERT_TRUE(check(*Analyzer, Answers));
}

TEST_F(TestDefAnalyzer, NonDebugLocN) {
  const std::string Code = "#include <gtest/gtest.h>\n"
                           "extern \"C\" { void API(); }\n"
                           "TEST(Test, Test) {\n"
                           "  API();\n"
                           "}\n";
  std::vector<AnswerT> Answers = {};
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::set<std::string> APINames = {"API"};
  auto Analyzer = load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp"),
                       "-O0 -w", APINames);
  ASSERT_TRUE(Analyzer);
  ASSERT_TRUE(check(*Analyzer, Answers));
}
