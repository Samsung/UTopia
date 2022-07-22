#include "TestHelper.h"
#include "ftg/generation/FuzzInput.h"
#include "testutil/APIManualLoader.h"

using namespace ftg;

class FuzzInputGenerationTest : public testing::Test {

protected:
  static std::shared_ptr<TATestHelper> TAHelper;
  static std::shared_ptr<UATestHelper> UAHelper;
  static std::unique_ptr<GenTestHelper> GenHelper;

  static void SetUpTestCase() {

    const std::string ProjectName = "test_fuzzinputgenerator";
    const std::string LibCode = "void API_1(int P1) {}\n"
                                "void API_2(const char *P2) {}\n"
                                "void API_3(int *P1) {}\n";

    TAHelper = TestHelperFactory().createTATestHelper(
        LibCode, "lib", CompileHelper::SourceType_C);
    ASSERT_TRUE(TAHelper);
    ASSERT_TRUE(TAHelper->analyze());

    DirectionAnalysisReport DirectionReport;
    DirectionReport.set("API_4", 0, Dir_Out);

    FilePathAnalysisReport FilePathReport;
    FilePathReport.set("API_2", 0);

    ArrayAnalysisReport ArrayReport;
    ArrayReport.set("API_6", 0, 1);

    std::vector<std::string> APINames = {"API_1", "API_2", "API_3", "API_4",
                                         "API_5", "API_6", "API_7"};
    std::vector<std::string> SrcPaths = {getUTSourcePath(ProjectName)};
    auto CH = TestHelperFactory().createCompileHelper(
        getProjectBaseDirPath(ProjectName), SrcPaths, "-O0 -g",
        CompileHelper::SourceType_C);
    ASSERT_TRUE(CH);

    auto AL = std::make_shared<APIManualLoader>(
        std::set<std::string>(APINames.begin(), APINames.end()));
    ASSERT_TRUE(AL);

    auto Loader = std::make_shared<UTLoader>(CH, AL);
    ASSERT_TRUE(Loader);
    Loader->setDirectionReport(DirectionReport);
    Loader->setFilePathReport(FilePathReport);
    Loader->setArrayReport(ArrayReport);

    UAHelper = std::make_shared<UATestHelper>();
    ASSERT_TRUE(UAHelper && UAHelper->analyze(Loader));

    GenHelper = std::make_unique<GenTestHelper>(UAHelper);
    ASSERT_TRUE(GenHelper->generate(getProjectBaseDirPath(ProjectName),
                                    getProjectOutDirPath(ProjectName),
                                    APINames));
  }

  static void TearDownTestCase() {

    TAHelper.reset();
    UAHelper.reset();
    GenHelper.reset();
  }

  const std::shared_ptr<Definition> getDef(unsigned Offset) {

    if (!GenHelper)
      return nullptr;
    for (auto DefMapIter :
         GenHelper->getGenerator().getLoader().getInputReport().getDefMap()) {
      const auto &Def = DefMapIter.second;
      if (!Def || Def->Offset != Offset)
        continue;
      return Def;
    }

    return nullptr;
  }

  const Fuzzer *getFuzzer(std::string Name) const {
    if (!GenHelper)
      return nullptr;
    auto *F = GenHelper->getGenerator().getFuzzer(Name);
    return F;
  }
};

std::shared_ptr<TATestHelper> FuzzInputGenerationTest::TAHelper = nullptr;
std::shared_ptr<UATestHelper> FuzzInputGenerationTest::UAHelper = nullptr;
std::unique_ptr<GenTestHelper> FuzzInputGenerationTest::GenHelper = nullptr;

TEST_F(FuzzInputGenerationTest, primitiveP) {

  auto Def = getDef(294);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_int");
  ASSERT_TRUE(F);
  ASSERT_TRUE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, filepathP) {

  auto Def = getDef(345);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_filepath");
  ASSERT_TRUE(F);
  ASSERT_TRUE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, fixedLenArrayP) {

  auto Def = getDef(402);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_fixedlen_array");
  ASSERT_TRUE(F);
  ASSERT_TRUE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, varLenArrayN) {

  auto Def = getDef(481);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_varlen_array");
  ASSERT_TRUE(F);
  ASSERT_FALSE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, nullptrN) {

  auto Def = getDef(536);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_nullptr");
  ASSERT_TRUE(F);
  ASSERT_FALSE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, reliedconstN) {

  auto Def = getDef(663);
  ASSERT_TRUE(Def);

  auto *F = getFuzzer("utc_reliedconst");
  ASSERT_TRUE(F);
  ASSERT_FALSE(F->isFuzzableInput(Def->ID));
}

TEST_F(FuzzInputGenerationTest, externallyAssignedN) {

  auto *F = getFuzzer("utc_externallyassigned");
  ASSERT_TRUE(F);

  auto Def1 = getDef(751);
  auto Def2 = getDef(788);
  ASSERT_TRUE(Def1);
  ASSERT_TRUE(Def2);

  ASSERT_FALSE(F->isFuzzableInput(Def1->ID));
  ASSERT_FALSE(F->isFuzzableInput(Def2->ID));
}

TEST_F(FuzzInputGenerationTest, array_propP) {

  {
    auto *F = getFuzzer("utc_array_property_pair");
    ASSERT_TRUE(F);

    auto Def1 = getDef(887);
    auto Def2 = getDef(909);
    ASSERT_TRUE(Def1);
    ASSERT_TRUE(Def2);

    ASSERT_TRUE(F->isFuzzableInput(Def1->ID));
    ASSERT_TRUE(F->isFuzzableInput(Def2->ID));
  }

  {
    auto *F = getFuzzer("utc_array_property_only_array");
    ASSERT_TRUE(F);

    auto Def = getDef(993);
    ASSERT_TRUE(Def);
    ASSERT_TRUE(F->isFuzzableInput(Def->ID));
  }
}

TEST_F(FuzzInputGenerationTest, array_propN) {

  {
    auto *F = getFuzzer("utc_array_property_only_arraylen");
    ASSERT_TRUE(F);

    auto Def = getDef(1108);
    ASSERT_TRUE(Def);
    ASSERT_FALSE(F->isFuzzableInput(Def->ID));
  }
}

TEST(TestFuzzInputFactory, NormalPtrHasArrayPropertyP) {
  auto IntT = std::make_shared<Type>(Type::TypeID_Integer);
  IntT->setASTTypeName("unsigned");
  auto ArrayArrayInfo = std::make_shared<ArrayInfo>();
  auto ArrayT = std::make_shared<Type>(Type::TypeID_Pointer);
  ArrayT->setASTTypeName("unsigned *");
  ArrayT->setArrayInfo(ArrayArrayInfo);
  ArrayT->setPointeeType(IntT);
  ArrayT->setPtrKind(Type::PtrKind::PtrKind_Normal);
  auto ArrayD = std::make_shared<Definition>();
  ArrayD->Array = true;
  ArrayD->DataType = ArrayT;
  ArrayD->ID = 0;
  std::shared_ptr<const Definition> InputD = ArrayD;
  auto ArrayInput = FuzzInputFactory().generate(InputD);
  ASSERT_TRUE(ArrayInput->getFTGType().isArrayPtr());
}

TEST(TestFuzzInputFactory, StringPtrP) {
  auto IntT = std::make_shared<Type>(Type::TypeID_Integer);
  IntT->setASTTypeName("unsigned");
  auto ArrayArrayInfo = std::make_shared<ArrayInfo>();
  auto ArrayT = std::make_shared<Type>(Type::TypeID_Pointer);
  ArrayT->setASTTypeName("unsigned *");
  ArrayT->setArrayInfo(ArrayArrayInfo);
  ArrayT->setPointeeType(IntT);
  ArrayT->setPtrKind(Type::PtrKind::PtrKind_String);
  auto ArrayD = std::make_shared<Definition>();
  ArrayD->Array = true;
  ArrayD->DataType = ArrayT;
  ArrayD->ID = 0;
  std::shared_ptr<const Definition> InputD = ArrayD;
  auto ArrayInput = FuzzInputFactory().generate(InputD);
  ASSERT_TRUE(ArrayInput->getFTGType().isStringType());
}

TEST(TestFuzzInputFactory, NullDefinitionN) {

  std::shared_ptr<const Definition> D;
  ASSERT_DEATH(FuzzInputFactory().generate(D), "");
}

TEST(TestFuzzInputFactory, NullTypeN) {
  auto D = std::make_shared<Definition>();
  std::shared_ptr<const Definition> InputD = D;
  ASSERT_DEATH(FuzzInputFactory().generate(InputD), "");
}
