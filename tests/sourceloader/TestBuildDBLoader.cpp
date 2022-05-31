//===-- TestBuildDBLoader.cpp - Unit tests for BuildDBLoader --------------===//

#include "TestHelper.h"
#include "ftg/sourceloader/BuildDBLoader.h"

#include <string>

using namespace ftg;

static const std::string TestBinary = "foo";

TEST(BuildDBLoaderDeathTest, TestLoadWhenProjectEntryNotExistN) {
  auto BuildDBPath = getProjectBaseDirPath("not_exist");
  EXPECT_DEATH(BuildDBLoader Loader(BuildDBPath, TestBinary), "not exist");
}

TEST(BuildDBLoaderDeathTest, TestLoadWhenCompileDBNotExistN) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  EXPECT_DEATH(BuildDBLoader Loader(BuildDBPath, "not_exists"), "not exists");
}
