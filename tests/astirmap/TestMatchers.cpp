#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/utils/ASTUtil.h"

using namespace ftg;

class TestNonStaticClassMethodMatcher : public ::testing::Test {

protected:
  static void SetUpTestCase() {
    auto CH = TestHelperFactory().createCompileHelper(
        CODE, "test_matchers", "-O0 -g", CompileHelper::SourceType_CPP);
    ASSERT_TRUE(CH);

    SC = CH->load();
    ASSERT_TRUE(SC);
  }

  bool exist(const std::vector<const clang::CXXMethodDecl *> &Decls,
             std::string Name) {

    return std::find_if(Decls.begin(), Decls.end(), [Name](const auto *D) {
             if (!D)
               return false;
             return D->getName() == Name;
           }) != Decls.end();
  }

  static std::unique_ptr<SourceCollection> SC;
  static const std::string CODE;
};

std::unique_ptr<SourceCollection> TestNonStaticClassMethodMatcher::SC = nullptr;

const std::string TestNonStaticClassMethodMatcher::CODE =
    "void Func1();\n"
    "static void Func2();\n"
    "class CLS1 {\n"
    "public:\n"
    "  void Method1();\n"
    "  static void Method2();\n"
    "};\n";

TEST_F(TestNonStaticClassMethodMatcher, NonStaticClassMethodsP) {
  ASSERT_TRUE(SC);
  auto Methods = util::collectNonStaticClassMethods(SC->getASTUnits());
  ASSERT_TRUE(exist(Methods, "Method1"));
}

TEST_F(TestNonStaticClassMethodMatcher, StaticClassMethodsN) {
  ASSERT_TRUE(SC);
  auto Methods = util::collectNonStaticClassMethods(SC->getASTUnits());
  ASSERT_FALSE(exist(Methods, "Func1"));
  ASSERT_FALSE(exist(Methods, "Func2"));
  ASSERT_FALSE(exist(Methods, "Method2"));
}
