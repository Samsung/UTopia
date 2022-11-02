#include "TestHelper.h"
#include "ftg/tcanalysis/TCTExtractor.h"
#include "testutil/SourceFileManager.h"

#include <string>

namespace ftg {

class TestTCTExtractor : public TestBase {
protected:
  const std::string TCTypeCode = R"(
    typedef void (*void_fun_ptr)(void);
    typedef void (*tc_fun_ptr)(void);
    typedef struct testcase_s {
      const char* name;
      tc_fun_ptr function;
      void_fun_ptr startup;
      void_fun_ptr cleanup;
    } testcase;
    )";
  const std::string TCArrayDefCode = R"(
    testcase tc_array[] = {
      { "utc_test", utc_test_body, utc_test_startup, utc_test_cleanup }
    };
    )";
  const std::string UTCDeclCode = R"(
    extern void utc_test_startup(void);
    extern void utc_test_body(void);
    extern void utc_test_cleanup(void);
    )";
  const std::string UTCDefCode = R"(
    void utc_test_startup() {}
    void utc_test_body() {}
    void utc_test_cleanup() {}
    )";
};

TEST_F(TestTCTExtractor, ExtractNormalCodeP) {
  const std::string NormalUTCCode =
      TCTypeCode + UTCDeclCode + TCArrayDefCode + UTCDefCode;
  ASSERT_TRUE(loadC(NormalUTCCode));
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 1);

  auto TCName = TCNames[0];
  ASSERT_TRUE(TCName.compare("utc_test_body") == 0);
  const auto *TC = Extractor->getTC(TCName);
  auto TCFuncNodes = TC->getTestSequence();
  ASSERT_TRUE(TCFuncNodes.size() > 0);
  ASSERT_TRUE(TCFuncNodes[0].getName().compare("utc_test_startup") == 0);
  ASSERT_TRUE(TCFuncNodes[1].getName().compare("utc_test_body") == 0);
  ASSERT_TRUE(TCFuncNodes[2].getName().compare("utc_test_cleanup") == 0);
  auto TCFuncs = TC->getLinks();
  ASSERT_TRUE(TCFuncNodes[0].getName().find("utc_test_startup") !=
              std::string::npos);
  ASSERT_TRUE(TCFuncNodes[1].getName().find("utc_test_body") !=
              std::string::npos);
  ASSERT_TRUE(TCFuncNodes[2].getName().find("utc_test_cleanup") !=
              std::string::npos);
}

TEST_F(TestTCTExtractor, ExtractWithNotExistTCNameN) {
  const std::string NormalUTCCode =
      TCTypeCode + UTCDeclCode + TCArrayDefCode + UTCDefCode;
  ASSERT_TRUE(loadC(NormalUTCCode));
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_FALSE(Extractor->getTC("NotExistTCName"));
}

TEST_F(TestTCTExtractor, ExtractWithEmptyCodeN) {
  const std::string EmptyCode = "";
  ASSERT_TRUE(loadC(EmptyCode));
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 0);
}

TEST_F(TestTCTExtractor, ExtractWithoutTCArrayN) {
  const std::string NoTCArrayCode = TCTypeCode + UTCDefCode;
  ASSERT_TRUE(loadC(NoTCArrayCode));
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 0);

  const auto *TC = Extractor->getTC("utc_test_body");
  ASSERT_FALSE(TC);
}

TEST_F(TestTCTExtractor, ExtractTCWithDeclaredPathP) {
  const std::string DeclCode = TCTypeCode + UTCDeclCode + TCArrayDefCode;
  const std::string HeaderInclude = R"(
    #include "header.h"
    )";
  const std::string DefCode = HeaderInclude + UTCDefCode;
  SourceFileManager SFM;
  SFM.createFile("header.h", DeclCode);
  SFM.createFile("source.c", DefCode);

  std::vector<std::string> SrcPaths = {SFM.getFilePath("source.c")};
  auto CH = TestHelperFactory().createCompileHelper(
      SFM.getSrcDirPath(), SrcPaths, "-O0 -g", CompileHelper::SourceType_C);
  ASSERT_TRUE(CH);

  auto SC = CH->load();
  ASSERT_TRUE(SC);

  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);

  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_EQ(TCNames.size(), 1);
  ASSERT_EQ(TCNames[0], "utc_test_body");

  const auto *TC = Extractor->getTC("utc_test_body");
  ASSERT_TRUE(TC);

  ASSERT_EQ(TC->getFilePath(), SFM.getFilePath("source.c"));
}

TEST_F(TestTCTExtractor, ExtractTCWithSeparatedFilesP) {
  const std::string DeclCode = TCTypeCode + UTCDeclCode + TCArrayDefCode;
  const std::string DefCode = UTCDefCode;
  SourceFileManager SFM;
  SFM.createFile("dec.c", DeclCode);
  SFM.createFile("def.c", UTCDefCode);

  std::vector<std::string> SrcPaths = {SFM.getFilePath("dec.c"),
                                       SFM.getFilePath("def.c")};
  auto CH = TestHelperFactory().createCompileHelper(
      SFM.getSrcDirPath(), SrcPaths, "-O0 -g", CompileHelper::SourceType_C);
  ASSERT_TRUE(CH);

  auto SC = CH->load();
  ASSERT_TRUE(SC);

  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);

  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_EQ(TCNames.size(), 1);
  ASSERT_EQ(TCNames[0], "utc_test_body");

  const auto *TC = Extractor->getTC("utc_test_body");
  ASSERT_TRUE(TC);

  ASSERT_EQ(TC->getFilePath(), SFM.getFilePath("def.c"));
}

TEST_F(TestTCTExtractor, ExtractTCWithoutDefinitionN) {
  const std::string DeclCode = TCTypeCode + UTCDeclCode + TCArrayDefCode;
  SourceFileManager SFM;
  SFM.createFile("dec.c", DeclCode);

  std::vector<std::string> SrcPaths = {SFM.getFilePath("dec.c")};
  auto CH = TestHelperFactory().createCompileHelper(
      SFM.getSrcDirPath(), SrcPaths, "-O0 -g", CompileHelper::SourceType_C);
  ASSERT_TRUE(CH);

  auto SC = CH->load();
  ASSERT_TRUE(SC);

  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);

  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_EQ(TCNames.size(), 0);
}

} // namespace ftg
