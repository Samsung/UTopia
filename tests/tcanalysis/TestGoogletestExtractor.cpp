#include "TestHelper.h"
#include "ftg/sourceloader/SourceCollection.h"
#include "ftg/tcanalysis/GoogleTestExtractor.h"

#include <string>

namespace ftg {

class TestGoogletestExtractor : public TestBase {
protected:
  void assertGoogletestTeststepWithoutEnvironment(
      const std::vector<llvm::Function *> *TestStep) {
    ASSERT_TRUE(TestStep->size() == 5);
    ASSERT_TRUE(TestStep->at(0)->getName().find("SetUpTestCase") !=
                std::string::npos);
    ASSERT_TRUE(TestStep->at(1)->getName().find("SetUp") != std::string::npos);
    ASSERT_TRUE(TestStep->at(2)->getName().find("TestBody") !=
                std::string::npos);
    ASSERT_TRUE(TestStep->at(3)->getName().find("TearDown") !=
                std::string::npos);
    ASSERT_TRUE(TestStep->at(4)->getName().find("TearDownTestCase") !=
                std::string::npos);
  }

  const std::string NormalCode =
      "#include <gtest/gtest.h>\n"
      "class TestExtractorSample : public testing::Test {\n"
      "void SetUp() {}\n"
      "void TearDown() {}\n"
      "};\n"
      "TEST(TestExtractorSample, PositiveSample1) {}\n"
      "TEST_F(TestExtractorSample, PositiveSample2) {}\n";
  const std::string EmptyCode = "";
};

TEST_F(TestGoogletestExtractor, ExtractNormalCodeP) {
  ASSERT_TRUE(loadCPP(NormalCode, "gtestextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 2);

  for (auto TCName : Extractor->getTCNames()) {
    const auto *TC = Extractor->getTC(TCName);
    auto TCFuncNodes = TC->getTestSequence();
    ASSERT_TRUE(TCFuncNodes.size() > 0);
    auto TCFuncs = TC->getLinks();
    assertGoogletestTeststepWithoutEnvironment(&TCFuncs);
  }
}

TEST_F(TestGoogletestExtractor, ExtractWithNotExistTCNameN) {
  ASSERT_TRUE(loadCPP(NormalCode, "gtestextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 2);
  ASSERT_FALSE(Extractor->getTC("NotExistTCName"));
}

TEST_F(TestGoogletestExtractor, ExtractWithEmptyCodeN) {
  ASSERT_TRUE(loadCPP(EmptyCode, "gtestextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 0);
}
} // namespace ftg
