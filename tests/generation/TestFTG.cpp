#include "TestHelper.h"
#include "testutil/APIManualLoader.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

class TestFTG : public ::testing::Test {
public:
  static const std::string ProjectName;

protected:
  static std::unique_ptr<GenTestHelper> Helper;
  static void SetUpTestCase() {
    std::shared_ptr<TATestHelper> TAPreHelper = nullptr;
    if (fs::exists(getPreSourcePath(ProjectName))) {
      std::vector<std::string> PreSrcPaths = {getPreSourcePath(ProjectName)};
      TAPreHelper = TestHelperFactory().createTATestHelper(
          getProjectBaseDirPath(ProjectName), PreSrcPaths,
          CompileHelper::SourceType_C);
      ASSERT_TRUE(TAPreHelper);
      ASSERT_TRUE(TAPreHelper->analyze());
    }
    std::vector<std::string> TargetSrcPaths = {
        getTargetSourcePath(ProjectName)};
    auto TALibHelper = TestHelperFactory().createTATestHelper(
        getProjectBaseDirPath(ProjectName), TargetSrcPaths,
        CompileHelper::SourceType_C);
    ASSERT_TRUE(TALibHelper);
    if (TAPreHelper) {
      ASSERT_TRUE(TALibHelper->addExternal(TAPreHelper));
    }
    ASSERT_TRUE(TALibHelper->analyze());
    TALibHelper->dumpReport("target.json");

    std::vector<std::string> APINames = {"inputInt",
                                         "inputUInt",
                                         "inputChar",
                                         "inputFloat",
                                         "inputDouble",
                                         "inputCStr",
                                         "inputIntPtr",
                                         "inputStructPtr",
                                         "inputEnumPtr",
                                         "outputPtr",
                                         "inputVoidPtr",
                                         "inputCallBackPtr",
                                         "inputCStrStrLen",
                                         "inputArr",
                                         "inputStringArr",
                                         "inputStructArr",
                                         "inputArrArrLen",
                                         "inputVoidArrArrLen",
                                         "outputArrArrLen",
                                         "inputEnum",
                                         "inputStruct",
                                         "inputUnion",
                                         "inputUnsupportedStruct",
                                         "filepath",
                                         "loopexit",
                                         "noop"};

    std::vector<std::string> UTSrcPaths = {getUTSourcePath(ProjectName)};
    auto UAHelper = TestHelperFactory().createUATestHelper(
        getProjectBaseDirPath(ProjectName), UTSrcPaths,
        CompileHelper::SourceType_C);
    ASSERT_TRUE(UAHelper);
    UAHelper->addTA(TALibHelper);
    if (TAPreHelper) {
      UAHelper->addTA(TAPreHelper);
    }
    ASSERT_TRUE(UAHelper->analyze(std::make_shared<APIManualLoader>(
        std::set<std::string>(APINames.begin(), APINames.end()))));

    Helper = std::make_unique<GenTestHelper>(UAHelper);
    ASSERT_TRUE(Helper);
    Helper->generate(getProjectBaseDirPath(ProjectName),
                     getProjectOutDirPath(ProjectName), APINames);
  }
  static void TearDownTestCase() { Helper.reset(); }
};
std::unique_ptr<GenTestHelper> TestFTG::Helper = nullptr;
const std::string TestFTG::ProjectName = "test_ftg";

#ifndef INCLUDE_NOT_FUZZABLE_TC
TEST_F(TestFTG, DefinedTypeP) {

  std::set<std::pair<std::string, FuzzStatus>> APIAnswers = {
      {"inputEnum", FUZZABLE_SRC_GENERATED},
      {"inputStruct", FUZZABLE_SRC_GENERATED},
  };

  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();

  for (auto Iter : APIAnswers) {
    auto &APIReport = Reporter.getAPIReport(Iter.first);
    ASSERT_TRUE(APIReport);
    EXPECT_EQ(APIReport->APIStatus.second, Iter.second);
  }

  std::set<std::pair<std::string, FuzzStatus>> UTAnswers = {
      {"utc_defined_type_p", FUZZABLE_SRC_GENERATED},
  };

  for (auto Iter : UTAnswers) {
    auto &UTReport = Reporter.getUTReport(Iter.first);
    ASSERT_TRUE(UTReport);
    EXPECT_EQ(UTReport->Status, Iter.second);
  }
}

TEST_F(TestFTG, MacroFuncAssignN) {
  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  auto &UTReport = Reporter.getUTReport("utc_macro_func_assign_n");
  ASSERT_TRUE(UTReport);
  EXPECT_EQ(UTReport->Status, NOT_FUZZABLE_UNIDENTIFIED);
}

TEST_F(TestFTG, VariableLengthArrayN) {
  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  auto UTReport = Reporter.getUTReport("utc_variable_length_array_n");
  ASSERT_EQ(NOT_FUZZABLE_UNIDENTIFIED, UTReport->Status);
}

#if !defined(__arm__) && !defined(__aarch64__)
// ARM compiler uses unsigned char for char type in default.
// It may cause verification error for .proto file.
// Do this verification only if ARM compiler is not used.
TEST_F(TestFTG, VerifyGeneratedP) { Helper->verifyGenerated(ProjectName); }
#endif // ARM compiler
#endif // INCLUDE_NOT_FUZZABLE_TC

TEST_F(TestFTG, FixedLengthArrayP) {
  ASSERT_TRUE(Helper);
  std::vector<std::string> TargetAPI = {"inputArr", "inputArrArrLen"};
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIName : TargetAPI) {
    auto APIReport = Reporter.getAPIReport(APIName);
    EXPECT_EQ(FUZZABLE_SRC_GENERATED, APIReport->APIStatus.second);
  }
#ifndef INCLUDE_NOT_FUZZABLE_TC
  auto UTReport = Reporter.getUTReport("utc_fixed_length_array_p");
  EXPECT_EQ(FUZZABLE_SRC_GENERATED, UTReport->Status);
#endif
}

TEST_F(TestFTG, FixedLengthArrayN) {
  ASSERT_TRUE(Helper);
  std::vector<std::string> TargetAPI = {"inputStringArr", "inputStructArr"};
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIName : TargetAPI) {
    auto APIReport = Reporter.getAPIReport(APIName);
    EXPECT_EQ(NOT_FUZZABLE_UNIDENTIFIED, APIReport->APIStatus.second);
  }
#ifndef INCLUDE_NOT_FUZZABLE_TC
  auto UTReport = Reporter.getUTReport("utc_fixed_length_array_n");
  EXPECT_EQ(NOT_FUZZABLE_UNIDENTIFIED, UTReport->Status);
#endif
}

TEST_F(TestFTG, PrimitiveTypeP) {

  std::set<std::pair<std::string, FuzzStatus>> APIAnswers = {
      {"inputInt", FUZZABLE_SRC_GENERATED},
      {"inputUInt", FUZZABLE_SRC_GENERATED},
      {"inputChar", FUZZABLE_SRC_GENERATED},
      {"inputFloat", FUZZABLE_SRC_GENERATED},
      {"inputDouble", FUZZABLE_SRC_GENERATED}};

  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();

  for (auto Iter : APIAnswers) {
    auto &APIReport = Reporter.getAPIReport(Iter.first);
    ASSERT_TRUE(APIReport);
    EXPECT_EQ(APIReport->APIStatus.second, Iter.second);
  }

  std::set<std::pair<std::string, FuzzStatus>> UTAnswers = {
      {"utc_primitive_type_p", FUZZABLE_SRC_GENERATED},
  };
  for (auto Iter : UTAnswers) {
    auto &UTReport = Reporter.getUTReport(Iter.first);
    ASSERT_TRUE(UTReport);
    EXPECT_EQ(UTReport->Status, Iter.second);
  }
}

TEST_F(TestFTG, StrTypeP) {
  std::set<std::pair<std::string, FuzzStatus>> APIAnswers = {
      {"inputCStr", FUZZABLE_SRC_GENERATED}};

  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();

  for (auto Iter : APIAnswers) {
    auto &APIReport = Reporter.getAPIReport(Iter.first);
    ASSERT_TRUE(APIReport);
    EXPECT_EQ(APIReport->APIStatus.second, Iter.second);
  }

  std::set<std::pair<std::string, FuzzStatus>> UTAnswers = {
      {"utc_str_type_p", FUZZABLE_SRC_GENERATED},
  };
  for (auto Iter : UTAnswers) {
    auto &UTReport = Reporter.getUTReport(Iter.first);
    ASSERT_TRUE(UTReport);
    EXPECT_EQ(UTReport->Status, Iter.second);
  }
}

TEST_F(TestFTG, PointerTypeP) {
  std::set<std::pair<std::string, FuzzStatus>> APIAnswers = {
      {"inputIntPtr", FUZZABLE_SRC_GENERATED},
      {"inputStructPtr", NOT_FUZZABLE_UNIDENTIFIED},
      {"inputEnumPtr", FUZZABLE_SRC_GENERATED}};

  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();

  for (auto Iter : APIAnswers) {
    auto &APIReport = Reporter.getAPIReport(Iter.first);
    ASSERT_TRUE(APIReport);
    ASSERT_EQ(APIReport->APIStatus.second, Iter.second);
  }

  std::set<std::pair<std::string, FuzzStatus>> UTAnswers = {
      {"utc_pointer_type_p", FUZZABLE_SRC_GENERATED},
  };
  for (auto Iter : UTAnswers) {
    auto &UTReport = Reporter.getUTReport(Iter.first);
    ASSERT_TRUE(UTReport);
    EXPECT_EQ(UTReport->Status, Iter.second);
  }
}

TEST_F(TestFTG, UnsupportedTypeN) {

  std::set<std::string> TestAPISet{"inputVoidPtr", "inputUnion",
                                   "inputUnsupportedStruct", "inputCallBackPtr",
                                   "inputVoidArrArrLen"};

  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIReportPair : Reporter.getAPIReportMap()) {
    if (TestAPISet.find(APIReportPair.first) == TestAPISet.end())
      continue;
    auto APIReport = APIReportPair.second;

    EXPECT_EQ(NOT_FUZZABLE_UNIDENTIFIED, APIReport->APIStatus.second);
  }
}

TEST_F(TestFTG, NoInputN) {
  // FIXME: "outputArrArrLen"
  std::set<std::string> TestAPISet{"noop", "outputPtr"};
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIReportPair : Reporter.getAPIReportMap()) {
    if (TestAPISet.find(APIReportPair.first) == TestAPISet.end())
      continue;
    auto APIReport = APIReportPair.second;
    EXPECT_EQ(NOT_FUZZABLE_NO_INPUT, APIReport->APIStatus.second);
  }
}

} // namespace ftg
