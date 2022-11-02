#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/utils/ASTUtil.h"

using namespace ftg;

class TestNonStaticClassMethodMatcher : public TestBase {

protected:
  void SetUp() override {
    const std::string CODE = "void Func1();\n"
                             "static void Func2();\n"
                             "class CLS1 {\n"
                             "public:\n"
                             "  void Method1();\n"
                             "  static void Method2();\n"
                             "};\n";

    ASSERT_TRUE(loadCPP(CODE));
  }

  bool exist(const std::vector<const clang::CXXMethodDecl *> &Decls,
             std::string Name) {

    return std::find_if(Decls.begin(), Decls.end(), [Name](const auto *D) {
             if (!D)
               return false;
             return D->getName() == Name;
           }) != Decls.end();
  }
};

TEST_F(TestNonStaticClassMethodMatcher, NonStaticClassMethodsP) {
  auto Methods = util::collectNonStaticClassMethods(SC->getASTUnits());
  ASSERT_TRUE(exist(Methods, "Method1"));
}

TEST_F(TestNonStaticClassMethodMatcher, StaticClassMethodsN) {
  auto Methods = util::collectNonStaticClassMethods(SC->getASTUnits());
  ASSERT_FALSE(exist(Methods, "Func1"));
  ASSERT_FALSE(exist(Methods, "Func2"));
  ASSERT_FALSE(exist(Methods, "Method2"));
}
