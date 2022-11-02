#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "llvm/IR/InstrTypes.h"

namespace ftg {

class TestMacroMapper : public TestBase {};

TEST_F(TestMacroMapper, FunctionName) {

  const std::string CODE = "void API_1();\n"
                           "void test_func_name() {\n"
                           "#define NAME API_1\n"
                           "  NAME();\n"
                           "#undef NAME\n"
                           "}\n";

  ASSERT_TRUE(load(CODE, "-O0 -g", CompileHelper::SourceType_C));

  auto *I = IRAH->getInstruction("test_func_name", 0, 0);
  ASSERT_TRUE(I);

  auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(I);
  ASSERT_TRUE(CB);
  ASSERT_TRUE(AIMap->getASTDefNode(*CB));
}

} // end namespace ftg
