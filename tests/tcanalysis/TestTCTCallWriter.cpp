#include "TestHelper.h"
#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/GoogleTestCallWriter.h"
#include "ftg/tcanalysis/TCTCallWriter.h"
#include "ftg/tcanalysis/TCTExtractor.h"
#include <gtest/gtest.h>

namespace ftg {

class TestTCTCallWriter : public TestBase {
protected:
  void SetUp() override {
    ASSERT_TRUE(loadC(TCTCode));
    Extractor = std::make_unique<TCTExtractor>(*SC.get());
    ASSERT_TRUE(Extractor);
    Extractor->load();
    ASSERT_TRUE(Extractor->getTCNames().size() == 1);
    TCName = Extractor->getTCNames()[0];
  }

  const std::string TCTCode = "typedef void (*void_fun_ptr)(void);\n"
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
  std::string ExpectCallStr = "utc_test1_startup();\n"
                              "utc_test1_body();\n"
                              "utc_test1_cleanup();\n";

  std::unique_ptr<TCExtractor> Extractor;
  std::string TCName;
};

TEST_F(TestTCTCallWriter, getUTMacroReplaceStringP) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> CallWriter = std::make_unique<TCTCallWriter>();

  auto Result = CallWriter->getTCCall(*TC);
  ASSERT_TRUE(Result.compare(ExpectCallStr) == 0);
}

TEST_F(TestTCTCallWriter, getUTMacroReplaceStringWithOtherCallWriterN) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> OtherCallWriter =
      std::make_unique<GoogleTestCallWriter>();

  auto Result = OtherCallWriter->getTCCall(*TC);
  ASSERT_FALSE(Result.compare(ExpectCallStr) == 0);
}
} // namespace ftg
