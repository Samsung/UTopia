#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/constantanalysis/ASTValue.h"

using namespace clang;

namespace ftg {

namespace TestASTValue {

class TestASTValueBase : public TestBase {

protected:
  std::pair<clang::Expr *, clang::ASTContext *>
  getASTFromInst(std::string Func, unsigned BIdx, unsigned IIdx) {
    if (!AIMap || !IRAH)
      return std::make_pair(nullptr, nullptr);

    auto *I = IRAH->getInstruction(Func, BIdx, IIdx);
    if (!I)
      return std::make_pair(nullptr, nullptr);

    auto *ADN = AIMap->getASTDefNode(*I);
    if (!ADN)
      return std::make_pair(nullptr, nullptr);

    auto *Assigned = ADN->getAssigned();
    if (!Assigned)
      return std::make_pair(nullptr, nullptr);

    return std::make_pair(const_cast<Expr *>(Assigned->getNode().get<Expr>()),
                          &Assigned->getASTUnit().getASTContext());
  }
};

class TestASTValue : public TestASTValueBase {

protected:
  std::pair<clang::Expr *, clang::ASTContext *>
  getASTFromArgument(std::string Func, unsigned BIdx, unsigned IIdx,
                     unsigned AIdx) {
    auto *I = IRAH->getInstruction(Func, BIdx, IIdx);
    if (!I || !isa<llvm::CallBase>(I))
      return std::make_pair(nullptr, nullptr);

    auto *ADN = AIMap->getASTDefNode(*I, AIdx);
    if (!ADN)
      return std::make_pair(nullptr, nullptr);

    auto *Assigned = ADN->getAssigned();
    if (!Assigned)
      return std::make_pair(nullptr, nullptr);

    return std::make_pair(
        const_cast<clang::Expr *>(Assigned->getNode().get<clang::Expr>()),
        &Assigned->getASTUnit().getASTContext());
  }

  bool checkValue(clang::Expr &E, clang::ASTContext &Ctx,
                  ASTValueData::VType AnswerType,
                  std::string AnswerValue) const {

    ASTValue V1(E, Ctx);

    return V1.getValues()[0] == ASTValueData(AnswerType, AnswerValue);
  }

  bool checkValue(clang::Expr &E, clang::ASTContext &Ctx,
                  std::vector<ASTValueData> Answer) const {

    ASTValue V1(E, Ctx);
    return (V1.getValues() == Answer) && (V1.isArray() == true);
  }
};

TEST_F(TestASTValue, DeclUninitP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1;\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 0);
  ASSERT_FALSE(AST.first);
}

TEST_F(TestASTValue, IntP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1 = 10;\n"
                           "  int Var2 = -10;\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 3);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "10"));

  AST = getASTFromInst("test", 0, 5);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "-10"));
}

TEST_F(TestASTValue, FloatP) {

  const std::string CODE = "void test() {\n"
                           "  float Var1 = 10.3;\n"
                           "  float Var2 = 10.3f;\n"
                           "  float Var3 = -10.3;\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 4);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(checkValue(*AST.first, *AST.second, ASTValueData::VType_FLOAT,
                         "10.300000"));

  AST = getASTFromInst("test", 0, 6);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(checkValue(*AST.first, *AST.second, ASTValueData::VType_FLOAT,
                         "10.300000"));

  AST = getASTFromInst("test", 0, 8);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(checkValue(*AST.first, *AST.second, ASTValueData::VType_FLOAT,
                         "-10.300000"));
}

TEST_F(TestASTValue, StringP) {

  const std::string CODE = "void test() {\n"
                           "  const char *Var1 = \"Hello\";\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_STRING, "Hello"));
}

TEST_F(TestASTValue, NegativeValueP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1 = -1;\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "-1"));
}

TEST_F(TestASTValue, CastP) {

  const std::string CODE = "void test() {\n"
                           "  unsigned Var1 = -10;\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(checkValue(*AST.first, *AST.second, ASTValueData::VType_INT,
                         "4294967286"));
}

TEST_F(TestASTValue, ArgumentP) {

  const std::string CODE = "void func1(int P1);\n"
                           "void test() {\n"
                           "  func1(10);\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromArgument("test", 0, 0, 0);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "10"));
}

TEST_F(TestASTValue, SizeOfP) {

  const std::string CODE = "struct ST1 { int F1; int F2; char F3; };\n"
                           "void test() {\n"
                           "  int Var1 = sizeof(struct ST1);\n"
                           "  char Var2[10];\n"
                           "  int Var3 = sizeof(*Var2);\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 4);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "12"));

  AST = getASTFromInst("test", 0, 7);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "1"));
}

TEST_F(TestASTValue, EnumP) {

  const std::string CODE =
      "enum EN1 { EN1_E1 = 10, EN1_E2, EN1_E3 = 0, EN1_E4 };\n"
      "void test() {\n"
      "  enum EN1 Var1 = EN1_E1;\n"
      "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "10"));
}

TEST_F(TestASTValue, EnumClassP) {

  const std::string CODE = "enum class EN { EN1, EN2, EN3, EN4 };\n"
                           "void test() {\n"
                           "  EN Var1 = EN::EN3;\n"
                           "}";

  ASSERT_TRUE(loadCPP(CODE));

  auto AST = getASTFromInst("_Z4testv", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "2"));
}

TEST_F(TestASTValue, ConstantExprP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1 = 10 * (20 + 30);\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "500"));
}

TEST_F(TestASTValue, StaticConstantP) {

  const std::string CODE = "class CLS { public: static int F1; };\n"
                           "int CLS::F1 = 10;\n"
                           "void test() {\n"
                           "  int Var1 = CLS::F1;\n"
                           "}\n";

  ASSERT_TRUE(loadCPP(CODE));

  auto AST = getASTFromInst("_Z4testv", 0, 3);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(
      checkValue(*AST.first, *AST.second, ASTValueData::VType_INT, "10"));
}

TEST_F(TestASTValue, StringEscapeP) {

  const std::string CODE = "void test() {\n"
                           "  const char *Var1 = \"\\\\0\\\\n\\0\";\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto AST = getASTFromInst("test", 0, 2);
  ASSERT_TRUE(AST.first && AST.second);
  ASSERT_TRUE(checkValue(*AST.first, *AST.second, ASTValueData::VType_STRING,
                         "\\\\0\\\\n\\0"));
}

TEST_F(TestASTValue, ArrayP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1[] = { 1, 2 };\n"
                           "  float Var2[] = { 1.1, 2.2 };\n"
                           "  char* Var3[] = { \"Hello\", \"World\" };\n"
                           "}";

  ASSERT_TRUE(loadC(CODE));

  {
    auto AST = getASTFromInst("test", 0, 5);
    ASSERT_TRUE(AST.first && AST.second);

    std::vector<ASTValueData> Answer;
    Answer.emplace_back(ASTValueData::VType_INT, "1");
    Answer.emplace_back(ASTValueData::VType_INT, "2");

    ASSERT_TRUE(checkValue(*AST.first, *AST.second, Answer));
  }

  {
    auto AST = getASTFromInst("test", 0, 8);
    ASSERT_TRUE(AST.first && AST.second);

    std::vector<ASTValueData> Answer;
    Answer.emplace_back(ASTValueData::VType_FLOAT, "1.100000");
    Answer.emplace_back(ASTValueData::VType_FLOAT, "2.200000");

    ASSERT_TRUE(checkValue(*AST.first, *AST.second, Answer));
  }

  {
    auto AST = getASTFromInst("test", 0, 11);
    ASSERT_TRUE(AST.first && AST.second);

    std::vector<ASTValueData> Answer;
    Answer.emplace_back(ASTValueData::VType_STRING, "Hello");
    Answer.emplace_back(ASTValueData::VType_STRING, "World");

    ASSERT_TRUE(checkValue(*AST.first, *AST.second, Answer));
  }
}

class TestSerialize : public TestASTValueBase {

protected:
  bool check(std::string FuncName, unsigned BIdx, unsigned IIdx) {

    auto AST = getASTFromInst(FuncName, BIdx, IIdx);
    if (!AST.first || !AST.second)
      return false;

    ASTValue V1(*AST.first, *AST.second);
    ASTValue V2(V1.toJson());

    return V1 == V2;
  }
};

TEST_F(TestSerialize, NoneN) {

  ASTValue V1;
  ASTValue V2(V1.toJson());
  EXPECT_EQ(V1, V2);
}

TEST_F(TestSerialize, ArrayN) {

  const std::string CODE = "void test() {\n"
                           "  int Var1 = 10;\n"
                           "  float Var2 = 10.3;\n"
                           "  const char* Var3 = \"Hello\";\n"
                           "}";

  ASSERT_TRUE(loadC(CODE));

  check("test", 0, 4);
  check("test", 0, 6);
  check("test", 0, 8);
}

TEST_F(TestSerialize, ArrayP) {

  const std::string CODE = "void test() {\n"
                           "  int Var1[] = { 1, 2 };\n"
                           "  float Var2[] = { 1.1, 2.2 };\n"
                           "  char* Var3[] = { \"Hello\", \"World\" };\n"
                           "}";

  ASSERT_TRUE(loadC(CODE));

  check("test", 0, 5);
  check("test", 0, 8);
  check("test", 0, 11);
}

} // namespace TestASTValue

} // namespace ftg
