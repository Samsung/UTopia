#include "TestHelper.h"
#include "ftg/type/Type.h"
#include "ftg/utils/StringUtil.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "json/json.h"
#include <gtest/gtest.h>

using namespace clang;
using namespace ast_matchers;

namespace ftg {

class TestType : public TestBase {

protected:
  std::shared_ptr<Type> create(std::string TypeName) {
    auto Tag = "TypeTag";
    auto Matcher = qualType(asString(TypeName)).bind(Tag);
    auto *Context = getContext();
    if (!Context)
      return nullptr;

    for (auto &Node : match(Matcher, *Context)) {
      auto *Record = Node.getNodeAs<clang::QualType>(Tag);
      if (!Record)
        continue;

      auto Result = Type::createType(*Record, *Context);
      if (!Result)
        continue;

      return Result;
    }
    return nullptr;
  }

private:
  ASTContext *getContext() {
    if (!SC)
      return nullptr;

    for (const auto *ASTUnit : SC->getASTUnits()) {
      if (!ASTUnit)
        continue;

      return const_cast<ASTContext *>(&ASTUnit->getASTContext());
    }
    return nullptr;
  }
};

TEST_F(TestType, TypeP) {
  const std::string Code =
      "extern \"C\" {\n"
      "enum E1 { Enum0, Enum1 };\n"
      "void test_defined(E1 P4);\n"
      "void test_primitive(int P1, float P2);\n"
      "void test_ptr(int *P1, char *P2, int P3[10], int P4) {\n"
      "  int Var[P4];\n"
      "}\n"
      "}";
  ASSERT_TRUE(loadCPP(Code));
  std::shared_ptr<Type> T;

  T = create("int");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isIntegerType());

  T = create("float");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isFloatType());

  T = create("enum E1");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->getKind() == Type::TypeID_Enum);

  T = create("int *");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isPointerType());
  ASSERT_TRUE(T->isNormalPtr());

  T = create("char *");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isPointerType());
  ASSERT_TRUE(T->isStringType());

  T = create("int [10]");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isPointerType());
  ASSERT_TRUE(T->isFixedLengthArrayPtr());

  T = create("int [P4]");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isPointerType());
}

TEST_F(TestType, TypeN) {
  const std::string Code = "extern \"C\" {\n"
                           "void test_ptr(int P1[]);\n"
                           "}";
  ASSERT_TRUE(loadCPP(Code));
  std::shared_ptr<Type> T;

  T = create("int []");
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isPointerType());
  ASSERT_TRUE(!T->isFixedLengthArrayPtr());
}

TEST_F(TestType, UnknownTypeN) {
  const std::string Code =
      "extern \"C\" {\n"
      "struct ST1 { int F1; };\n"
      "union U1 { int F1; float F2; };\n"
      "class C1 { public: C1(int P1):F1(P1) {}; private: int F1; };\n"
      "void test_defined(ST1 *P1, U1 *P2, C1 *P3);\n"
      "void test_void();\n"
      "}";
  ASSERT_TRUE(loadCPP(Code));
  std::shared_ptr<Type> T;

  T = create("void");
  ASSERT_FALSE(T);

  T = create("struct ST1");
  ASSERT_FALSE(T);

  T = create("union U1");
  ASSERT_FALSE(T);

  T = create("class C1");
  ASSERT_FALSE(T);

  T = create("void (void)");
  ASSERT_FALSE(T);
}

TEST_F(TestType, CreateCharPointerTypeP) {
  auto T = Type::createCharPointerType();
  ASSERT_TRUE(T);
  ASSERT_TRUE(T->isStringType());
}

TEST_F(TestType, CreateCharPointerTypeN) {
  auto T = Type::createCharPointerType();
  ASSERT_TRUE(T);
  ASSERT_FALSE(T->isArrayPtr());
}

TEST_F(TestType, ToJsonP) {
  auto ExpectedJson = util::strToJson(R"(
    {
      "ASTTypeName" : "int",
      "TypeID" : 2,
      "NameSpace" : "",
      "TypeSize" : 4,
      "TypeName" : "",
      "GlobalDef" : null,
      "IsBoolean" : false,
      "IsUnsigned" : false,
      "IsAnyCharacter" : false,
      "PointerInfo" : null
    })");

  const std::string Code = "int var;";
  ASSERT_TRUE(loadCPP(Code));
  std::shared_ptr<Type> T = create("int");

  ASSERT_EQ(T->toJson().toStyledString(), ExpectedJson.toStyledString());
}

TEST_F(TestType, ToJsonWithGlobalDefP) {
  auto ExpectedJson = util::strToJson(R"(
    {
      "ASTTypeName" : "E",
      "TypeID" : 1,
      "NameSpace" : "",
      "TypeSize" : 0,
      "TypeName" : "E",
      "IsBoolean" : false,
      "IsUnsigned" : true,
      "IsAnyCharacter" : false,
      "PointerInfo" : null,
      "GlobalDef" : {
        "Name" : "enum E",
        "Enumerators" : [
          { "Name" : "A", "Value" : 0 }
        ]
      }
    })");

  std::vector<EnumConst> Enumerators;
  Enumerators.emplace_back("A", 0);
  auto E = std::make_shared<Enum>("enum E", Enumerators);

  const std::string Code = "enum E { A };";
  ASSERT_TRUE(loadCPP(Code));
  std::shared_ptr<Type> T = create("enum E");
  T->setGlobalDef(E);
  ASSERT_EQ(T->toJson().toStyledString(), ExpectedJson.toStyledString());
}

TEST_F(TestType, InitFromJsonP) {
  Json::Value TypeJson = util::strToJson(R"(
        {
          "ASTTypeName" : "int",
          "TypeID" : 2,
          "NameSpace" : "",
          "TypeSize" : 4,
          "TypeName" : "",
          "GlobalDef" : null,
          "IsBoolean" : false,
          "IsUnsigned" : false,
          "IsAnyCharacter" : false,
          "PointerInfo" : null
          }
      )");

  Type T(TypeJson);
  ASSERT_TRUE(T.fromJson(TypeJson));
  ASSERT_EQ("int", T.getASTTypeName());
  ASSERT_EQ(Type::TypeID_Integer, T.getKind());
  ASSERT_EQ("", T.getNameSpace());
  ASSERT_EQ(4, T.getTypeSize());
  ASSERT_EQ("", T.getTypeName());
  ASSERT_EQ(nullptr, T.getGlobalDef());
  ASSERT_EQ(false, T.isBoolean());
  ASSERT_EQ(false, T.isUnsigned());
  ASSERT_EQ(false, T.isAnyCharacter());
}

TEST_F(TestType, FromJsonP) {
  Json::Value TypeJson = util::strToJson(R"(
    {
      "ASTTypeName" : "int",
      "TypeID" : 2,
      "NameSpace" : "",
      "TypeSize" : 4,
      "TypeName" : "",
      "GlobalDef" : null,
      "IsBoolean" : false,
      "IsUnsigned" : false,
      "IsAnyCharacter" : false,
      "PointerInfo" : null
    })");

  auto T = Type::createCharPointerType();
  ASSERT_TRUE(T->fromJson(TypeJson));
  ASSERT_EQ("int", T->getASTTypeName());
  ASSERT_EQ(Type::TypeID_Integer, T->getKind());
  ASSERT_EQ("", T->getNameSpace());
  ASSERT_EQ(4, T->getTypeSize());
  ASSERT_EQ("", T->getTypeName());
  ASSERT_EQ(nullptr, T->getGlobalDef());
  ASSERT_EQ(false, T->isBoolean());
  ASSERT_EQ(false, T->isUnsigned());
  ASSERT_EQ(false, T->isAnyCharacter());
}

TEST_F(TestType, FromJsonWithGlobalDefP) {
  auto TargetLibJson = util::strToJson(R"(
    {
      "typeanalysis": {
        "enums": {
          "E": {
            "Enumerators": [
              {"Name": "A", "Value": 0}
            ],
            "Name": "enum _E"
          }
        }
      }
    })");

  Json::Value TypeJson = util::strToJson(R"(
    {
      "ASTTypeName" : "E",
      "TypeID" : 1,
      "NameSpace" : "",
      "TypeSize" : 0,
      "TypeName" : "E",
      "IsBoolean" : false,
      "IsUnsigned" : true,
      "IsAnyCharacter" : false,
      "PointerInfo" : null,
      "GlobalDef" : {}
    })");

  TypeAnalysisReport TypeReport;
  ASSERT_TRUE(TypeReport.fromJson(TargetLibJson));
  auto T = Type::createCharPointerType();
  ASSERT_TRUE(T->fromJson(TypeJson, &TypeReport));
  ASSERT_EQ("E", T->getASTTypeName());
  ASSERT_EQ(Type::TypeID_Enum, T->getKind());
  ASSERT_EQ("", T->getNameSpace());
  ASSERT_EQ(0, T->getTypeSize());
  ASSERT_EQ("E", T->getTypeName());
  const auto *GDef = T->getGlobalDef();
  ASSERT_TRUE(GDef);
  auto EConsts = GDef->getElements();
  ASSERT_EQ(1, EConsts.size());
  auto EConst = EConsts[0];
  ASSERT_EQ("A", EConst.getName());
  ASSERT_EQ(0, EConst.getValue());
  ASSERT_EQ(false, T->isBoolean());
  ASSERT_EQ(true, T->isUnsigned());
  ASSERT_EQ(false, T->isAnyCharacter());
}

TEST_F(TestType, FromEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson(R"({})");

  auto T = Type::createCharPointerType();
  EXPECT_FALSE(T->fromJson(EmptyJson));
}

TEST_F(TestType, FromJsonNullValueN) {
  Json::Value EmptyJson = Json::nullValue;

  auto T = Type::createCharPointerType();
  EXPECT_FALSE(T->fromJson(EmptyJson));
}

class DummyType : public Type {
public:
  static PointerInfo createPointerInfo(PtrKind Kind) {
    return PointerInfo(Kind);
  }
  static PointerInfo createPointerInfo(Json::Value Json) {
    return PointerInfo(Json);
  }
};

class TestPointerInfo : public testing::Test {};

TEST_F(TestPointerInfo, toJsonP) {
  auto ExpectedJson = util::strToJson(R"(
    {
      "PointerKind": 0,
      "ArrayInfo" : null,
      "PointeeType" : null
    })");
  auto PInfo = DummyType::createPointerInfo(Type::PtrKind::PtrKind_Normal);
  ASSERT_EQ(PInfo.toJson().toStyledString(), ExpectedJson.toStyledString());
}

TEST_F(TestPointerInfo, fromJsonP) {
  auto PointerInfoJson = util::strToJson(R"(
    {
      "PointerKind": 0,
      "ArrayInfo" : null,
      "PointeeType" : {
        "ASTTypeName" : "int",
        "TypeID" : 2,
        "NameSpace" : "",
        "TypeSize" : 4,
        "TypeName" : "",
        "GlobalDef" : null,
        "IsBoolean" : false,
        "IsUnsigned" : false,
        "IsAnyCharacter" : false,
        "PointerInfo" : null
    }})");

  auto PInfo = DummyType::createPointerInfo(Type::PtrKind::PtrKind_Normal);
  PInfo.fromJson(PointerInfoJson);
  ASSERT_EQ(Type::PtrKind::PtrKind_Normal, PInfo.getPtrKind());
  ASSERT_FALSE(PInfo.getArrayInfo());
  ASSERT_TRUE(PInfo.getPointeeType());

  const auto *T = PInfo.getPointeeType();
  ASSERT_EQ("int", T->getASTTypeName());
  ASSERT_EQ(Type::TypeID_Integer, T->getKind());
  ASSERT_EQ("", T->getNameSpace());
  ASSERT_EQ(4, T->getTypeSize());
  ASSERT_EQ("", T->getTypeName());
  ASSERT_EQ(nullptr, T->getGlobalDef());
  ASSERT_EQ(false, T->isBoolean());
  ASSERT_EQ(false, T->isUnsigned());
  ASSERT_EQ(false, T->isAnyCharacter());
}

TEST_F(TestPointerInfo, fromNullPointeeTypeJsonN) {
  auto PointerInfoJson = util::strToJson(R"(
    {
      "PointerKind": 0,
      "ArrayInfo" : null,
      "PointeeType" : null
    })");

  auto PInfo = DummyType::createPointerInfo(Type::PtrKind::PtrKind_Normal);
  PInfo.fromJson(PointerInfoJson);
  ASSERT_EQ(Type::PtrKind::PtrKind_Normal, PInfo.getPtrKind());
  ASSERT_FALSE(PInfo.getArrayInfo());
  ASSERT_FALSE(PInfo.getPointeeType());
}

TEST_F(TestPointerInfo, FromEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson(R"({})");

  auto PInfo = DummyType::createPointerInfo(Type::PtrKind::PtrKind_Normal);
  EXPECT_FALSE(PInfo.fromJson(EmptyJson));
}

TEST_F(TestPointerInfo, FromJsonNullValueN) {
  auto PInfo = DummyType::createPointerInfo(Type::PtrKind::PtrKind_Normal);
  EXPECT_FALSE(PInfo.fromJson(Json::nullValue));
}

TEST_F(TestPointerInfo, InitFromJson) {
  auto PointerInfoJson = util::strToJson(R"(
    {
      "PointerKind": 0,
      "ArrayInfo" : null,
      "PointeeType" : null
    })");

  auto PInfo = DummyType::createPointerInfo(PointerInfoJson);
  ASSERT_EQ(Type::PtrKind::PtrKind_Normal, PInfo.getPtrKind());
  ASSERT_FALSE(PInfo.getArrayInfo());
  ASSERT_FALSE(PInfo.getPointeeType());
}

TEST(TestPointerInfoDeathTest, FromEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson(R"({})");

  ASSERT_DEATH(DummyType::createPointerInfo(EmptyJson),
               "Unexpected Program State");
}

TEST(TestPointerInfoDeathTest, FromJsonNullValueN) {
  ASSERT_DEATH(DummyType::createPointerInfo(Json::nullValue),
               "Unexpected Program State");
}

} // namespace ftg
