#include "TestHelper.h"
#include "ftg/tcanalysis/BoostCallWriter.h"
#include "ftg/tcanalysis/BoostExtractor.h"
#include "ftg/tcanalysis/GoogleTestCallWriter.h"

#include <string>

namespace ftg {

class TestBoostCallWriter : public TestBase {
protected:
  void SetUp() override {
    ASSERT_TRUE(loadCPP(BoostCode, "boostcallwriter"));

    std::unique_ptr<SourceCollection> SC = CH->load();
    ASSERT_TRUE(SC);
    Extractor = std::make_unique<BoostExtractor>(*SC.get());
    ASSERT_TRUE(Extractor);
    Extractor->load();
    ASSERT_TRUE(Extractor->getTCNames().size() == 1);
    TCName = Extractor->getTCNames()[0];
  }

  const std::string BoostCode = "#define BOOST_TEST_DYN_LINK\n"
                                "#define BOOST_TEST_MAIN\n"
                                "#include \"boost/test/unit_test.hpp\"\n"
                                "BOOST_AUTO_TEST_CASE(test) {}\n";
  std::string ExpectCallStr = "try {\n"
                              "  struct test Fuzzer;\n"
                              "  Fuzzer.test_method();\n"
                              "} catch (std::exception &E) {}\n";

  std::unique_ptr<TCExtractor> Extractor;
  std::string TCName;
};

TEST_F(TestBoostCallWriter, getUTMacroReplaceStringP) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> CallWriter =
      std::make_unique<BoostCallWriter>();

  auto Result = CallWriter->getTCCall(*TC);
  ASSERT_TRUE(Result.compare(ExpectCallStr) == 0);
}

TEST_F(TestBoostCallWriter, getUTMacroReplaceStringWithOtherCallWriterN) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> OtherCallWriter =
      std::make_unique<GoogleTestCallWriter>();

  auto Result = OtherCallWriter->getTCCall(*TC);
  ASSERT_FALSE(Result.compare(ExpectCallStr) == 0);
}
} // namespace ftg
