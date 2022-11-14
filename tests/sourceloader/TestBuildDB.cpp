//===-- TestBuildDB.cpp - Unit tests for BuildDB --------------------------===//

#include "TestHelper.h"
#include "ftg/sourceloader/BuildDB.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;
using namespace ftg;

TEST(TestBuildDB, TestLoadP) {
  auto DataDirPath = getProjectBaseDirPath("test_builddb");
  auto BuildDBPath = (fs::path(DataDirPath) / ("builddb.json")).string();
  auto DB = BuildDB::fromJson(BuildDBPath);
  ASSERT_EQ(DB->getBCPath(), "bc_file_path");
  ASSERT_EQ(DB->getProjectDir(), "project_dir_path");
  ASSERT_EQ(DB->getASTPaths().size(), 2);
}

TEST(TestBuildDB, TestLoadWithInvalidFormatN) {
  auto DataDirPath = getProjectBaseDirPath("test_builddb");
  auto BuildDBPath =
      (fs::path(DataDirPath) / ("builddb_invalid.json")).string();
  auto DB = BuildDB::fromJson(BuildDBPath);
  ASSERT_EQ(DB, nullptr);
}
