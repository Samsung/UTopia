#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "llvm/IR/InstrTypes.h"

namespace ftg {

class TestMacroMapper : public ::testing::Test {

protected:
  std::unique_ptr<ASTIRMap> AIMap;
  std::unique_ptr<SourceCollection> SC;
  std::unique_ptr<IRAccessHelper> IRAccess;

  bool load(std::string CODE, bool IsC) {
    CompileHelper::SourceType Type =
        IsC ? CompileHelper::SourceType_C : CompileHelper::SourceType_CPP;
    auto Compile = TestHelperFactory().createCompileHelper(CODE, "test_macro",
                                                           "-O0 -g", Type);
    if (!Compile)
      return false;

    SC = Compile->load();
    if (!SC)
      return false;

    IRAccess = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!IRAccess)
      return false;

    AIMap = std::make_unique<DebugInfoMap>(*SC);
    if (!AIMap)
      return false;

    return true;
  }
};

TEST_F(TestMacroMapper, FunctionName) {

  const std::string CODE = "void API_1();\n"
                           "void test_func_name() {\n"
                           "#define NAME API_1\n"
                           "  NAME();\n"
                           "#undef NAME\n"
                           "}\n";

  ASSERT_TRUE(load(CODE, true));

  auto *I = IRAccess->getInstruction("test_func_name", 0, 0);
  ASSERT_TRUE(I);

  auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(I);
  ASSERT_TRUE(CB);
  ASSERT_TRUE(AIMap->getASTDefNode(*CB));
}

} // end namespace ftg
