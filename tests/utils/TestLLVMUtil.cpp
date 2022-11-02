#include "TestHelper.h"
#include "ftg/utils/LLVMUtil.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;

class TestLLVMUtil : public ::testing::Test {
protected:
  std::shared_ptr<CompileHelper> CH;
  std::unique_ptr<IRAccessHelper> IRAccess;

  bool load(std::string SrcDir, std::string CodePath) {
    return load(SrcDir, std::vector<std::string>({CodePath}));
  }

  bool load(std::string SrcDir, std::vector<std::string> CodePaths) {
    CH = TestHelperFactory().createCompileHelper(SrcDir, CodePaths, "-O0 -g -w",
                                                 CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    auto *M = CH->getLLVMModule();
    if (!M)
      return false;

    IRAccess = std::make_unique<IRAccessHelper>(*M);
    return !!IRAccess;
  }
};

TEST_F(TestLLVMUtil, GetCalledFunction_GeneralP) {
  const std::string Code = "extern \"C\" {\n"
                           "  void API();\n"
                           "  void test() { API(); }\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));
  const auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 0));
  ASSERT_TRUE(CB);
  const auto *CF = util::getCalledFunction(*CB);
  ASSERT_TRUE(CF);
  ASSERT_EQ(CF->getName(), "API");
}

TEST_F(TestLLVMUtil, GetCalledFunction_IndirectN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API();\n"
                           "void test() {\n"
                           "  auto *V = &API;\n"
                           "  V();\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp")));
  const auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 4));
  ASSERT_TRUE(CB);
  const auto *CF = util::getCalledFunction(*CB);
  ASSERT_FALSE(CF);
}
