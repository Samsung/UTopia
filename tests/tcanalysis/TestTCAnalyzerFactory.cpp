#include "TestHelper.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"

#include <string>

namespace ftg {

class TestTCAnalyzerFactory : public TestBase {
protected:
  std::string TemporalCode = "";
};

TEST_F(TestTCAnalyzerFactory, CreateFactoryP) {
  ASSERT_TRUE(loadCPP(TemporalCode, "boostanalyzerfactory"));
  std::unique_ptr<SourceCollection> BoostSC = CH->load();

  TCAnalyzerFactory *AnalyzerFactory = new BoostAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> BoostExtractor =
      AnalyzerFactory->createTCExtractor(*BoostSC.get());
  ASSERT_TRUE(BoostExtractor);
  std::unique_ptr<TCCallWriter> BoostCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(BoostCallWriter);

  ASSERT_TRUE(loadCPP(TemporalCode, "googletestanalyzerfactory"));
  std::unique_ptr<SourceCollection> GoogleTestSC = CH->load();
  AnalyzerFactory = new GoogleTestAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> GoogleTestExtractor =
      AnalyzerFactory->createTCExtractor(*GoogleTestSC.get());
  ASSERT_TRUE(GoogleTestExtractor);
  std::unique_ptr<TCCallWriter> GoogleTestCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(GoogleTestCallWriter);

  ASSERT_TRUE(loadC(TemporalCode, "tctanalyzerfactory"));
  std::unique_ptr<SourceCollection> TCTSC = CH->load();
  AnalyzerFactory = new TCTAnalyzerFactory();
  ASSERT_TRUE(AnalyzerFactory);
  std::unique_ptr<TCExtractor> TCTExtractor =
      AnalyzerFactory->createTCExtractor(*TCTSC.get());
  ASSERT_TRUE(TCTExtractor);
  std::unique_ptr<TCCallWriter> TCTCallWriter =
      AnalyzerFactory->createTCCallWriter();
  ASSERT_TRUE(TCTCallWriter);
}

} // namespace ftg
