#include "TestInputAnalysisBase.hpp"
#include "ftg/inputanalysis/DefAnalyzer.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"
#include "testutil/SourceFileManager.h"

class TestInputAnalysisReport : public TestInputAnalysisBase {
protected:
  std::unique_ptr<InputAnalyzer> Analyzer;

  bool init(std::string SrcDir, std::string CodePath,
            std::vector<std::string> APINames) {
    if (!load(SrcDir, CodePath))
      return false;

    auto Factory = createTCAnalyzerFactory("gtest");
    if (!Factory)
      return false;

    auto Extractor = Factory->createTCExtractor(Loader->getSourceCollection());
    if (!Extractor)
      return false;

    std::vector<Unittest> UTs;
    Extractor->load();
    for (auto &TCName : Extractor->getTCNames()) {
      const auto *UT = Extractor->getTC(TCName);
      if (!UT)
        continue;

      UTs.push_back(*UT);
    }

    std::set<llvm::Function *> APIFuncs;
    const auto &M = Loader->getSourceCollection().getLLVMModule();
    for (const auto &APIName : APINames) {
      const auto *F = M.getFunction(APIName);
      if (!F)
        continue;

      APIFuncs.insert(const_cast<llvm::Function *>(F));
    }

    Analyzer = std::make_unique<DefAnalyzer>(
        Loader.get(), UTs, APIFuncs,
        std::make_shared<DebugInfoMap>(Loader->getSourceCollection()));
    return !!Analyzer;
  }
};

TEST_F(TestInputAnalysisReport, fromJsonP) {
  const std::string Code = "#include <gtest/gtest.h>\n"
                           "extern \"C\" {\n"
                           "void API1(int);\n"
                           "TEST(Test, Test) {\n"
                           "  int Var1 = 1;\n"
                           "  API1(Var1);\n"
                           "}"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(init(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp"), {"API1"}));

  InputAnalysisReport NewInputReport;
  TypeAnalysisReport EmptyTypeReport;
  auto SerializedReport = Analyzer->get().toJson();
  NewInputReport.fromJson(SerializedReport, EmptyTypeReport);
  ASSERT_EQ(SerializedReport, NewInputReport.toJson());
}

TEST_F(TestInputAnalysisReport, fromJsonN) {
  InputAnalysisReport Report;
  Json::Value EmptyValue;
  TypeAnalysisReport EmptyTypeReport;
  ASSERT_DEATH(Report.fromJson(EmptyValue), "Not Implemented");
  ASSERT_TRUE(Report.fromJson(EmptyValue, EmptyTypeReport));
}
