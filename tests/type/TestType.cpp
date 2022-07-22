#include "TestHelper.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"

using namespace clang;
using namespace ast_matchers;

namespace ftg {

class TestType : public testing::Test {

protected:
  std::unique_ptr<SourceCollection> SC;

  bool load(const std::string &Code) {
    auto CH = TestHelperFactory().createCompileHelper(
        Code, "type", "-O0 -g", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    return true;
  }

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
  ASSERT_TRUE(load(Code));
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
  ASSERT_TRUE(load(Code));
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
  ASSERT_TRUE(load(Code));
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

} // namespace ftg
