#include "TestHelper.h"
#include "ftg/tcanalysis/BoostExtractor.h"

namespace ftg {

class TestBoostExtractor : public TestBase {
protected:
  const std::string NormalTestCode = "#define BOOST_TEST_DYN_LINK\n"
                                     "#define BOOST_TEST_MAIN\n"
                                     "#include \"boost/test/unit_test.hpp\"\n"
                                     "BOOST_AUTO_TEST_CASE(test) {}\n";
  const std::string TemplateTestCode =
      "#define BOOST_TEST_DYN_LINK\n"
      "#define BOOST_TEST_MAIN\n"
      "#include \"boost/test/unit_test.hpp\"\n"
      "#include \"boost/mpl/vector.hpp\"\n"
      "BOOST_AUTO_TEST_CASE_TEMPLATE(template_test, T, "
      "boost::mpl::vector<int>) {}\n";
  const std::string EmptyCode = "";
};

TEST_F(TestBoostExtractor, ExtractNormalCodeP) {
  ASSERT_TRUE(loadCPP(NormalTestCode, "boostextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<BoostExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 1);

  auto TCName = TCNames[0];
  ASSERT_TRUE(TCName.compare("test") == 0);

  const auto *TC = Extractor->getTC(TCName);
  auto TCFuncNodes = TC->getTestSequence();
  ASSERT_TRUE(TCFuncNodes.size() > 0);
  ASSERT_TRUE(TCFuncNodes[0].getName().compare("test::test_method") == 0);
  auto TCFuncs = TC->getLinks();
  ASSERT_TRUE(TCFuncNodes[0].getName().find("test_method") !=
              std::string::npos);
}

TEST_F(TestBoostExtractor, NotExtractTemplateTestN) {
  ASSERT_TRUE(loadCPP(TemplateTestCode, "boostextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<BoostExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_TRUE(Extractor->getTCNames().size() == 0);
}

TEST_F(TestBoostExtractor, ExtractWithNotExistTCNameN) {
  ASSERT_TRUE(loadCPP(NormalTestCode, "boostextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<BoostExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  ASSERT_FALSE(Extractor->getTC("NotExistTCName"));
}

TEST_F(TestBoostExtractor, ExtractWithEmptyCodeN) {
  ASSERT_TRUE(loadC(EmptyCode, "tctextractor"));
  std::unique_ptr<SourceCollection> SC = CH->load();
  auto Extractor = std::make_unique<BoostExtractor>(*SC.get());
  ASSERT_TRUE(Extractor);
  Extractor->load();

  auto TCNames = Extractor->getTCNames();
  ASSERT_TRUE(TCNames.size() == 0);
}

} // namespace ftg
