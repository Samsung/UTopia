#include "TestHelper.h"
#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/targetanalysis/TargetLibExportUtil.h"
#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "testutil/APIManualLoader.h"

using namespace ftg;
using namespace clang;

class StructTest : public ::testing::Test {

protected:
  std::unique_ptr<TargetLib> loadLib(std::string CODE, bool IsC) {

    auto SourceType =
        IsC ? CompileHelper::SourceType_C : CompileHelper::SourceType_CPP;
    auto Compile = TestHelperFactory().createCompileHelper(
        CODE, "test_astvalue", "-O0 -g", SourceType);
    if (!Compile)
      return nullptr;

    TargetLibAnalyzer Analyzer(Compile, std::make_shared<APIManualLoader>());
    if (!Analyzer.analyze())
      return nullptr;

    TargetLibLoader Loader;
    Loader.load(toJsonString(Analyzer.getTargetLib()));

    return Loader.takeReport();
  }

  std::shared_ptr<Struct> getStruct(TargetLib &Context,
                                    std::string Name) const {

    auto StructMap = Context.getStructMap();
    auto Iter = StructMap.find(Name);
    if (Iter == StructMap.end())
      return nullptr;

    return Iter->second;
  }
};

TEST_F(StructTest, NonParamConstructor) {

  const std::string CODE = "struct ST {\n"
                           "  ST() : F1(20) {};\n"
                           "  int F1;\n"
                           "};";

  auto Context = loadLib(CODE, false);
  ASSERT_TRUE(Context);

  auto Struct = getStruct(*Context, "ST");
  ASSERT_TRUE(Struct);
}

TEST_F(StructTest, DefaultConstructor) {

  const std::string CODE = "struct ST1 {\n"
                           "  int getF1() { return F1; };\n"
                           "  int F1;\n"
                           "};\n"
                           "struct ST2 {\n"
                           "  int F1;\n"
                           "};\n";

  auto Context = loadLib(CODE, false);
  ASSERT_TRUE(Context);

  auto Struct = getStruct(*Context, "ST1");
  ASSERT_TRUE(Struct);

  Struct = getStruct(*Context, "ST2");
  ASSERT_TRUE(Struct);
}

TEST_F(StructTest, NoNonParamConstructor) {

  const std::string CODE = "struct ST {\n"
                           "  ST(int P1) : F1(P1) {};\n"
                           "  int F1;\n"
                           "};";

  auto Context = loadLib(CODE, false);
  ASSERT_TRUE(Context);

  auto Struct = getStruct(*Context, "ST");
  ASSERT_FALSE(Struct);
}

TEST_F(StructTest, PrivateMemberStruct) {

  const std::string CODE = "struct ST {\n"
                           "public:\n"
                           "  int F1;\n"
                           "private:\n"
                           "  int F2;\n"
                           "};";

  auto Context = loadLib(CODE, false);
  ASSERT_TRUE(Context);

  auto Struct = getStruct(*Context, "ST");
  ASSERT_FALSE(Struct);
}
