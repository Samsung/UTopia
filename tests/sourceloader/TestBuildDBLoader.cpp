#include "TestHelper.h"
#include "ftg/sourceloader/BuildDBLoader.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;

TEST(TestBuildDBLoader, EmptyPathN) {
  BuildDBLoader Loader("");
  ASSERT_DEATH(Loader.load(), "");
}

TEST(TestBuildDBLoader, InvalidASTN) {
  const std::string BuildDB =
      R"({"ast": ["invalid"],"bc" : "","project_dir" : ""})";
  SourceFileManager SFM;
  SFM.createFile("BuildDB.json", BuildDB);
  BuildDBLoader Loader(SFM.getFilePath("BuildDB.json"));
  ASSERT_DEATH(Loader.load(), "");
}

TEST(TestBuildDBLoader, EmptyBCN) {
  const std::string BuildDB =
      R"({"ast": [],"bc": "invalid","project_dir" : ""})";
  SourceFileManager SFM;
  SFM.createFile("BuildDB.json", BuildDB);
  BuildDBLoader Loader(SFM.getFilePath("BuildDB.json"));
  ASSERT_DEATH(Loader.load(), "");
}
