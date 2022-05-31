#include "TestHelper.h"
#include "ftg/tcanalysis/TCTExtractor.h"

#include <string>

namespace ftg {

class TestTCTExtractor : public TestBase {
protected:
  const std::string NormalCode = "typedef void (*void_fun_ptr)(void);\n"
                                 "typedef void (*tc_fun_ptr)(void);\n"
                                 "typedef struct testcase_s {\n"
                                 "  const char* name;\n"
                                 "  tc_fun_ptr function;\n"
                                 "  void_fun_ptr startup;\n"
                                 "  void_fun_ptr cleanup;\n"
                                 "} testcase;\n"
                                 "void utc_test1_startup() {}\n"
                                 "void utc_test1_body() {}\n"
                                 "void utc_test1_cleanup() {}\n"
                                 "testcase tc_array[] = {\n"
                                 "  { \"utc_test1\", utc_test1_body, "
                                 "utc_test1_startup, utc_test1_cleanup }\n"
                                 "};\n";
  const std::string NoTCArrayCode = "typedef void (*void_fun_ptr)(void);\n"
                                    "typedef void (*tc_fun_ptr)(void);\n"
                                    "typedef struct testcase_s {\n"
                                    "  const char* name;\n"
                                    "  tc_fun_ptr function;\n"
                                    "  void_fun_ptr startup;\n"
                                    "  void_fun_ptr cleanup;\n"
                                    "} testcase;\n"
                                    "void utc_test1_startup() {}\n"
                                    "void utc_test1_body() {}\n"
                                    "void utc_test1_cleanup() {}\n";
  const std::string EmptyCode = "";
};

TEST_F(TestTCTExtractor, ExtractNormalCodeP) {
  ASSERT_TRUE(loadC(NormalCode, "tctextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 1);

  auto TCName = TCNames[0];
  ASSERT_TRUE(TCName.compare("utc_test1_body") == 0);
  const auto *TC = Extractor->getTC(TCName);
  auto TCFuncNodes = TC->getTestSequence();
  ASSERT_TRUE(TCFuncNodes.size() > 0);
  ASSERT_TRUE(TCFuncNodes[0].getName().compare("utc_test1_startup") == 0);
  ASSERT_TRUE(TCFuncNodes[1].getName().compare("utc_test1_body") == 0);
  ASSERT_TRUE(TCFuncNodes[2].getName().compare("utc_test1_cleanup") == 0);
  auto TCFuncs = TC->getLinks();
  ASSERT_TRUE(TCFuncNodes[0].getName().find("utc_test1_startup") !=
              std::string::npos);
  ASSERT_TRUE(TCFuncNodes[1].getName().find("utc_test1_body") !=
              std::string::npos);
  ASSERT_TRUE(TCFuncNodes[2].getName().find("utc_test1_cleanup") !=
              std::string::npos);
}

TEST_F(TestTCTExtractor, ExtractWithNotExistTCNameN) {
  ASSERT_TRUE(loadC(NormalCode, "tctextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_FALSE(Extractor->getTC("NotExistTCName"));
}

TEST_F(TestTCTExtractor, ExtractWithEmptyCodeN) {
  ASSERT_TRUE(loadC(EmptyCode, "tctextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 0);
}

TEST_F(TestTCTExtractor, ExtractWithoutTCArrayN) {
  ASSERT_TRUE(loadC(NoTCArrayCode, "tctextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<TCTExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 0);

  const auto *TC = Extractor->getTC("utc_test1_body");
  ASSERT_FALSE(TC);
}
} // namespace ftg
