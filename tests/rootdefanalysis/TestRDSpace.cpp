#include "TestHelper.h"
#include "ftg/rootdefanalysis/RDSpace.h"
#include "testutil/IRAccessHelper.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;
using namespace llvm;

class TestRDSpace : public ::testing::Test {
protected:
  std::unique_ptr<IRAccessHelper> AH;
  std::unique_ptr<SourceCollection> SC;

  bool load(std::string BaseDir, std::vector<std::string> Paths) {
    for (auto Path : Paths)
      outs() << Path << "\n";
    auto CH = TestHelperFactory().createCompileHelper(
        BaseDir, Paths, "-O0", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    AH = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!AH)
      return false;

    return true;
  }
};

TEST_F(TestRDSpace, NullBuildN) {
  RDSpace Space;
  ASSERT_THROW(Space.build({nullptr}), std::invalid_argument);
};

TEST_F(TestRDSpace, NonBuildN) {
  const std::string Code = "extern \"C\" {\n"
                           "int test(int P) {\n"
                           "  return 0;\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), {SFM.getFilePath("test.cpp")}));

  auto *F = AH->getFunction("test");
  ASSERT_TRUE(F);

  RDSpace Space;
  auto Nexts = Space.nextInCallee(*F);
  ASSERT_EQ(Nexts.size(), 0);
};

TEST_F(TestRDSpace, NextInCallee_ReturnValuesP) {
  const std::string Code = "extern \"C\" {\n"
                           "int test(int P) {\n"
                           "  return 0;\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(load(SFM.getSrcDirPath(), {SFM.getFilePath("test.cpp")}));

  auto *F = AH->getFunction("test");
  ASSERT_TRUE(F);

  std::vector<llvm::Function *> Funcs = {F};
  RDSpace Space;
  Space.build(Funcs);
  auto Nexts = Space.nextInCallee(*F);
  ASSERT_EQ(Nexts.size(), 1);
  ASSERT_NE(Nexts.find(AH->getInstruction("test", 0, 2)), Nexts.end());
};
