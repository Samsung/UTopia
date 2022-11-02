#include "TestHelper.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"

#include <string>

namespace ftg {

class TestTCAnalyzerFactory : public TestBase {
protected:
  std::string TemporalCode = "";
};

TEST_F(TestTCAnalyzerFactory, CreateFactoryP) {
  ASSERT_TRUE(loadCPP(TemporalCode));

  TCAnalyzerFactory *AnalyzerFactory = new BoostAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> BoostExtractor =
      AnalyzerFactory->createTCExtractor(*SC.get());
  ASSERT_TRUE(BoostExtractor);
  std::unique_ptr<TCCallWriter> BoostCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(BoostCallWriter);

  ASSERT_TRUE(loadCPP(TemporalCode));
  AnalyzerFactory = new GoogleTestAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> GoogleTestExtractor =
      AnalyzerFactory->createTCExtractor(*SC.get());
  ASSERT_TRUE(GoogleTestExtractor);
  std::unique_ptr<TCCallWriter> GoogleTestCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(GoogleTestCallWriter);

  ASSERT_TRUE(loadC(TemporalCode));
  AnalyzerFactory = new TCTAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> TCTExtractor =
      AnalyzerFactory->createTCExtractor(*SC.get());
  ASSERT_TRUE(TCTExtractor);
  std::unique_ptr<TCCallWriter> TCTCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(TCTCallWriter);
}

} // namespace ftg
