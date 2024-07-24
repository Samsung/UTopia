#include "TestHelper.h"
#include "ftg/type/GlobalDef.h"
#include "ftg/utils/StringUtil.h"
#include "testutil/SourceFileManager.h"

namespace ftg {

TEST(TestGlobalDef, EnumConstToJsonP) {
  Json::Value ExpectedJson = util::strToJson(R"(
        {"Name" : "A", "Value" : 0}
      )");
  EnumConst Const("A", 0);
  ASSERT_EQ(Const.toJson().toStyledString(), ExpectedJson.toStyledString());
}

TEST(TestGlobalDef, EnumConstFromJsonP) {
  Json::Value EnumConstJson = util::strToJson(R"(
        {"Name" : "A", "Value" : 0}
      )");

  EnumConst Const(EnumConstJson);
  ASSERT_EQ("A", Const.getName());
  ASSERT_EQ(0, Const.getValue());
}

TEST(TestGlobalDef, EnumConstFromEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson(R"({})");

  EnumConst Const("", 0);
  EXPECT_FALSE(Const.fromJson(EmptyJson));
}

TEST(TestGlobalDef, EnumConstFromJsonNullValueN) {
  EnumConst Const("", 0);
  EXPECT_FALSE(Const.fromJson(Json::nullValue));
}

TEST(TestGlobalDef, EnumConstFromIncompleteJsonN) {
  EnumConst Const("", 0);

  Json::Value IncompleteEnumConstJson = util::strToJson(R"(
        {"Name" : "A"}
      )");
  EXPECT_FALSE(Const.fromJson(IncompleteEnumConstJson));

  IncompleteEnumConstJson = util::strToJson(R"(
        {"Value" : "0"}
      )");
  EXPECT_FALSE(Const.fromJson(IncompleteEnumConstJson));
}

TEST(TestGlobalDefDeathTest, EnumConstConstructWithJsonNullValueN) {
  ASSERT_DEATH(EnumConst(Json::nullValue);, "Unexpected Program State");
}

TEST(TestGlobalDef, EnumToJsonP) {
  Json::Value ExpectedJson = util::strToJson(R"(
      {
        "Name" : "enum",
        "Enumerators" : [
          {"Name" : "A", "Value" : 0}
        ],
      })");
  std::vector<EnumConst> Enumerators;
  Enumerators.emplace_back("A", 0);
  Enum E("enum", Enumerators);

  ASSERT_EQ(E.toJson().toStyledString(), ExpectedJson.toStyledString());
}

TEST(TestGlobalDef, EnumFromJsonP) {
  Json::Value EnumJson = util::strToJson(R"(
      {
        "Name" : "enum",
        "Enumerators" : [
          {"Name" : "A", "Value" : 0}
        ],
      })");
  Enum E(EnumJson);

  ASSERT_EQ("enum", E.getName());
  auto Const = E.getElements()[0];
  ASSERT_EQ("A", Const.getName());
  ASSERT_EQ(0, Const.getValue());
}

TEST(TestGlobalDef, EnumFromEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson(R"({})");

  Enum E("", std::vector<EnumConst>());
  EXPECT_FALSE(E.fromJson(EmptyJson));
}

TEST(TestGlobalDef, EnumFromJsonNullValueN) {
  Enum E("", std::vector<EnumConst>());
  EXPECT_FALSE(E.fromJson(Json::nullValue));
}

TEST(TestGlobalDef, EnumFromIncompleteJsonN) {
  Enum E("", std::vector<EnumConst>());

  Json::Value IncompleteEnumJson = util::strToJson(R"(
        {"Name" : "A"}
      )");
  EXPECT_FALSE(E.fromJson(IncompleteEnumJson));

  IncompleteEnumJson = util::strToJson(R"(
        {"Enumerators" : "[]"}
      )");
  EXPECT_FALSE(E.fromJson(IncompleteEnumJson));

  auto EnumeratorIsNotArrayJson = util::strToJson(R"(
        {"Name" : "A", "Enumerators" : 0}
      )");
  EXPECT_FALSE(E.fromJson(EnumeratorIsNotArrayJson));
}

TEST(TestGlobalDefDeathTest, EnumConstructWithJsonNullValueN) {
  ASSERT_DEATH(EnumConst(Json::nullValue), "Unexpected Program State");
}

TEST(TestGlobalDefEnum, AnonymousEnumP) {
  std::string Code = "enum { M1, M2 };\n";
  SourceFileManager SFM;
  ASSERT_TRUE(SFM.createFile("test.cpp", Code));

  std::vector<std::string> CodePaths = {SFM.getFilePath("test.cpp")};
  auto CH = TestHelperFactory().createCompileHelper(
      SFM.getSrcDirPath(), CodePaths, "-O0 -g -w",
      CompileHelper::SourceType_CPP);
  ASSERT_TRUE(CH);

  auto SC = CH->load();
  ASSERT_TRUE(SC);

  const auto &Units = SC->getASTUnits();
  ASSERT_EQ(Units.size(), 1);

  auto *Unit = Units[0];
  ASSERT_TRUE(Unit);

  auto &Ctx = Unit->getASTContext();
  auto Enum = createEnum("", Ctx);
  ASSERT_TRUE(Enum);
#if LLVM_VERSION_MAJOR < 17
  ASSERT_EQ(Enum->getName(), "(anonymous)");
#endif
}

} // namespace ftg
