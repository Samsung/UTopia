#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/astirmap/IRNode.h"
#include "testutil/IRAccessHelper.h"
#include "testutil/SourceFileManager.h"
#include <llvm/Support/raw_ostream.h>

using namespace ftg;

class TestIRNode : public ::testing::Test {
protected:
  std::unique_ptr<IRAccessHelper> AH;
  std::unique_ptr<DebugInfoMap> DIMap;
  std::unique_ptr<SourceCollection> SC;

  bool load(std::string BaseDirPath, std::string CodePath, std::string Opt) {
    std::vector<std::string> CodePaths = {CodePath};
    auto CH = TestHelperFactory().createCompileHelper(
        BaseDirPath, CodePaths, Opt, CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    if (!SC)
      return false;

    AH = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!AH)
      return false;

    DIMap = std::make_unique<DebugInfoMap>(*SC);
    return !!DIMap;
  }
};

TEST_F(TestIRNode, NonDebugLocN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API();\n"
                           "void test() { API(); }\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), SFM.getFilePath("test.cpp"), "-O0 -w"));
  auto *I = AH->getInstruction("test", 0, 0);
  ASSERT_TRUE(I);
  ASSERT_THROW((void)IRNode(*I), std::runtime_error);
}
