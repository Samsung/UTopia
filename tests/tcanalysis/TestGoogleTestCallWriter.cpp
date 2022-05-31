#include "TestHelper.h"
#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/GoogleTestCallWriter.h"
#include "ftg/tcanalysis/GoogleTestExtractor.h"
#include "ftg/tcanalysis/TCTCallWriter.h"
#include <gtest/gtest.h>

namespace ftg {

class TestGoogleTestCallWriter : public TestBase {
protected:
  void SetUp() override {
    ASSERT_TRUE(loadCPP(GoogleTestCode, "gtestcallwriter"));

    std::unique_ptr<SourceCollection> SC = CH->load();
    ASSERT_TRUE(SC);
    Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
    ASSERT_TRUE(Extractor);
    Extractor->load();
    ASSERT_TRUE(Extractor->getTCNames().size() == 1);
    TCName = Extractor->getTCNames()[0];
  }

  const std::string GoogleTestCode =
      "#include <gtest/gtest.h>\n"
      "class Test2 : public testing::Test {\n"
      "void SetUp() {}\n"
      "void TearDown() {}\n"
      "};\n"
      "TEST(CallWriterTest, PositiveSample) {}\n";
  std::string ExpectCallStr =
      "class AutofuzzTest : public CallWriterTest_PositiveSample_Test {\n"
      "public:\n"
      "  void runTest() {\n"
      "    try {\n"
      "      SetUpTestCase();\n"
      "    } catch (std::exception &E) {}\n"
      "    try {\n"
      "      SetUp();\n"
      "    } catch (std::exception &E) {}\n"
      "    try {\n"
      "      TestBody();\n"
      "    } catch (std::exception &E) {}\n"
      "    try {\n"
      "      TearDown();\n"
      "    } catch (std::exception &E) {}\n"
      "    try {\n"
      "      TearDownTestCase();\n"
      "    } catch (std::exception &E) {}\n"
      "  }\n"
      "};\n"
      "AutofuzzTest Fuzzer;\n"
      "Fuzzer.runTest();\n";

  std::unique_ptr<TCExtractor> Extractor;
  std::string TCName;
};

TEST_F(TestGoogleTestCallWriter, getUTMacroReplaceStringP) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> CallWriter =
      std::make_unique<GoogleTestCallWriter>();

  auto Result = CallWriter->getTCCall(*TC);
  ASSERT_TRUE(Result.compare(ExpectCallStr) == 0);
}

TEST_F(TestGoogleTestCallWriter, getUTMacroReplaceStringWithOtherCallWriterN) {
  const Unittest *TC = Extractor->getTC(TCName);
  ASSERT_TRUE(TC);
  std::unique_ptr<TCCallWriter> OtherCallWriter =
      std::make_unique<TCTCallWriter>();

  auto Result = OtherCallWriter->getTCCall(*TC);
  ASSERT_FALSE(Result.compare(ExpectCallStr) == 0);
}
} // namespace ftg
