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
  ASSERT_TRUE(loadCPP(NormalCode));
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
  ASSERT_TRUE(loadCPP(NormalCode));
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 2);
  ASSERT_FALSE(Extractor->getTC("NotExistTCName"));
}

TEST_F(TestGoogletestExtractor, ExtractWithEmptyCodeN) {
  ASSERT_TRUE(loadCPP(EmptyCode));
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 0);
}

TEST_F(TestGoogletestExtractor, GTEST_TYPED_TESTP) {
  std::string Code = "#include <gtest/gtest.h>\n"
                     "template <typename T>\n"
                     "class FooTest : public testing::Test {};\n"
                     "using MyTypes = ::testing::Types<char, int>;\n"
                     "TYPED_TEST_CASE(FooTest, MyTypes);\n"
                     "TYPED_TEST(FooTest, DoesBlah) {}\n";
  ASSERT_TRUE(loadCPP(Code));
  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC);
  Extractor->load();
  const auto Names = Extractor->getTCNames();
  ASSERT_EQ(Names.size(), 1);
  ASSERT_EQ(Names[0], "FooTest_DoesBlah_Test");
}

// NOTE : This TC is for the state when GTEST environment class is defined
// however not used. This causes that Environment class is not compiled into IR,
// however still related tokens are found on AST.
TEST_F(TestGoogletestExtractor, environmentN) {
  std::string Code = "#include <gtest/gtest.h>\n"
                     "class Environment : public ::testing::Environment{\n"
                     "public:\n"
                     "  void SetUp() override {}\n"
                     "};\n"
                     "TEST(Test, Test) {}\n";
  ASSERT_TRUE(loadCPP(Code));

  auto Extractor = std::make_unique<GoogleTestExtractor>(*SC);
  Extractor->load();
  const auto Names = Extractor->getTCNames();
  ASSERT_EQ(Names.size(), 1);
  ASSERT_EQ(Names[0], "Test_Test_Test");

  const auto *TC = Extractor->getTC("Test_Test_Test");
  ASSERT_TRUE(TC);
  ASSERT_EQ(TC->getLinks().size(), 5);
}

} // namespace ftg
