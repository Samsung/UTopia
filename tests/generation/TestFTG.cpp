#include "TestHelper.h"
#include "ftg/generation/ProtobufMutator.h"
#include "testutil/APIManualLoader.h"
#include "testutil/SourceFileManager.h"
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
    UAHelper->dumpReport("ut.json");

    Helper = std::make_unique<GenTestHelper>(UAHelper);
    ASSERT_TRUE(Helper);
    Helper->generate(getProjectBaseDirPath(ProjectName),
                     getProjectOutDirPath(ProjectName), APINames);
  }
  static void TearDownTestCase() { Helper.reset(); }
};
std::unique_ptr<GenTestHelper> TestFTG::Helper = nullptr;
const std::string TestFTG::ProjectName = "test_ftg";

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
#if LLVM_VERSION_MAJOR < 17
  EXPECT_EQ(UTReport->Status, NOT_FUZZABLE_UNIDENTIFIED);
#else
  EXPECT_EQ(UTReport->Status, FUZZABLE_SRC_GENERATED);
#endif
}

TEST_F(TestFTG, VariableLengthArrayN) {
  ASSERT_TRUE(Helper);
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  auto UTReport = Reporter.getUTReport("utc_variable_length_array_n");
#if LLVM_VERSION_MAJOR < 17
  ASSERT_EQ(NOT_FUZZABLE_UNIDENTIFIED, UTReport->Status);
#else
  ASSERT_EQ(FUZZABLE_SRC_GENERATED, UTReport->Status);
#endif
}

#if !defined(__arm__) && !defined(__aarch64__)
#if LLVM_VERSION_MAJOR < 17
// ARM compiler uses unsigned char for char type in default.
// It may cause verification error for .proto file.
// Do this verification only if ARM compiler is not used.
TEST_F(TestFTG, VerifyGeneratedP) { Helper->verifyGenerated(ProjectName); }
#endif
#endif // ARM compiler

TEST_F(TestFTG, FixedLengthArrayP) {
  ASSERT_TRUE(Helper);
  std::vector<std::string> TargetAPI = {"inputArr", "inputArrArrLen"};
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIName : TargetAPI) {
    auto APIReport = Reporter.getAPIReport(APIName);
    EXPECT_EQ(FUZZABLE_SRC_GENERATED, APIReport->APIStatus.second);
  }

  auto UTReport = Reporter.getUTReport("utc_fixed_length_array_p");
  EXPECT_EQ(FUZZABLE_SRC_GENERATED, UTReport->Status);
}

TEST_F(TestFTG, FixedLengthArrayN) {
  ASSERT_TRUE(Helper);
  std::vector<std::string> TargetAPI = {"inputStringArr", "inputStructArr"};
  auto &Reporter = Helper->getGenerator().getFuzzGenReporter();
  for (auto APIName : TargetAPI) {
    auto APIReport = Reporter.getAPIReport(APIName);
    EXPECT_EQ(NOT_FUZZABLE_UNIDENTIFIED, APIReport->APIStatus.second);
  }

  auto UTReport = Reporter.getUTReport("utc_fixed_length_array_n");
  EXPECT_EQ(NOT_FUZZABLE_UNIDENTIFIED, UTReport->Status);
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

  std::set<std::string> TestAPISet{"inputUnion", "inputUnsupportedStruct",
                                   "inputCallBackPtr"};

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

class TestIntegration : public ::testing::Test {
protected:
};

TEST_F(TestIntegration, cppCopyFromP) {
#if LLVM_VERSION_MAJOR < 17
  const std::string HeaderCode = "extern \"C\" void API(int *P1, int P2);\n";
  const std::string TargetCode = "#include \"lib.h\"\n"
                                 "extern \"C\" void API(int *P1, int P2) {\n"
                                 "  for (int S = 0; S < P2; ++S) {\n"
                                 "    P1[S] += S;\n"
                                 "  }\n"
                                 "}";
  const std::string UTCode = "#include \"lib.h\"\n"
                             "#include <gtest/gtest.h>\n"
                             "TEST(Test, Group) {\n"
                             "  int Var[10] = {0,};\n"
                             "  if (Var[0] == 0) API(Var, 10);\n"
                             "  else API(Var, 5);\n"
                             "}\n";
  SourceFileManager SFM;
  SFM.createFile("lib.h", HeaderCode);
  SFM.createFile("lib.cpp", TargetCode);
  SFM.createFile("ut.cpp", UTCode);

  std::vector<std::string> SrcPaths = {SFM.getFilePath("lib.cpp")};
  auto TAHelper = TestHelperFactory().createTATestHelper(
      SFM.getSrcDirPath(), SrcPaths, CompileHelper::SourceType_CPP);
  ASSERT_TRUE(TAHelper);
  ASSERT_TRUE(TAHelper->analyze());

  std::vector<std::string> APINames = {"API"};
  SrcPaths = {SFM.getFilePath("ut.cpp")};
  auto UAHelper = TestHelperFactory().createUATestHelper(
      SFM.getSrcDirPath(), SrcPaths, CompileHelper::SourceType_CPP);
  ASSERT_TRUE(UAHelper);
  UAHelper->addTA(TAHelper);
  ASSERT_TRUE(
      UAHelper->analyze(std::make_shared<APIManualLoader>(std::set<std::string>(
                            APINames.begin(), APINames.end())),
                        "gtest"));

  auto Helper = std::make_unique<GenTestHelper>(UAHelper);
  ASSERT_TRUE(Helper);
  ASSERT_TRUE(
      Helper->generate(SFM.getSrcDirPath(), SFM.getOutDirPath(), APINames));

  std::vector<std::string> VerifyFiles;
  const std::set<std::string> GenFiles = {
      "ut.cpp", InputMutator::DefaultEntryName,
      ProtobufMutator::DescriptorName + ".proto"};
  const std::set<std::string> TestCaseNames = {"Test_Group_Test"};
  for (const auto &GenFile : GenFiles) {
    for (const auto &TestCaseName : TestCaseNames) {
      VerifyFiles.emplace_back((fs::path(SFM.getOutDirPath()) /
                                fs::path(TestCaseName) / fs::path(GenFile))
                                   .string());
    }
  }
  verifyFiles(VerifyFiles);
#endif
}

TEST_F(TestIntegration, cppEnumP) {
  const std::string HeaderCode = "enum E1 { E1_M1, E1_M2 };\n"
                                 "typedef enum { E2_M1, E2_M2 } E2;\n"
                                 "typedef E2 E3;\n"
                                 "typedef E3 E4;\n"
                                 "extern \"C\" void API1(E1 P);\n"
                                 "extern \"C\" void API2(E2 P);\n";
  const std::string TargetCode = "#include \"lib.h\"\n"
                                 "extern \"C\" void API1(E1 P) {}\n"
                                 "extern \"C\" void API2(E2 P) {}\n";
  const std::string UTCode = "#include \"lib.h\"\n"
                             "#include <gtest/gtest.h>\n"
                             "TEST(Test, Enum) {\n"
                             "  API1(E1_M1);\n"
                             "  API2(E2_M1);\n"
                             "  E3 Var1 = E2_M1;\n"
                             "  API2(Var1);\n"
                             "  E4 Var2 = E2_M1;\n"
                             "  API2(Var2);\n"
                             "}\n";
  SourceFileManager SFM;
  SFM.createFile("lib.h", HeaderCode);
  SFM.createFile("lib.cpp", TargetCode);
  SFM.createFile("ut.cpp", UTCode);

  std::vector<std::string> SrcPaths = {SFM.getFilePath("lib.cpp")};
  auto TAHelper = TestHelperFactory().createTATestHelper(
      SFM.getSrcDirPath(), SrcPaths, CompileHelper::SourceType_CPP);
  ASSERT_TRUE(TAHelper);
  ASSERT_TRUE(TAHelper->analyze());

  std::vector<std::string> APINames = {"API1", "API2"};
  SrcPaths = {SFM.getFilePath("ut.cpp")};
  auto UAHelper = TestHelperFactory().createUATestHelper(
      SFM.getSrcDirPath(), SrcPaths, CompileHelper::SourceType_CPP);
  ASSERT_TRUE(UAHelper);
  UAHelper->addTA(TAHelper);
  ASSERT_TRUE(
      UAHelper->analyze(std::make_shared<APIManualLoader>(std::set<std::string>(
                            APINames.begin(), APINames.end())),
                        "gtest"));

  auto Helper = std::make_unique<GenTestHelper>(UAHelper);
  ASSERT_TRUE(Helper);
  ASSERT_TRUE(
      Helper->generate(SFM.getSrcDirPath(), SFM.getOutDirPath(), APINames));

  std::vector<std::string> VerifyFiles;
  const std::set<std::string> GenFiles = {
      "ut.cpp", InputMutator::DefaultEntryName,
      ProtobufMutator::DescriptorName + ".proto"};
  const std::set<std::string> TestCaseNames = {"Test_Enum_Test"};
  for (const auto &GenFile : GenFiles) {
    for (const auto &TestCaseName : TestCaseNames) {
      VerifyFiles.emplace_back((fs::path(SFM.getOutDirPath()) /
                                fs::path(TestCaseName) / fs::path(GenFile))
                                   .string());
    }
  }
  verifyFiles(VerifyFiles);
}

} // namespace ftg
