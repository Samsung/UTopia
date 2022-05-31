//===-- TestProjectEntry.cpp - Unit tests for ProjectEntry ----------------===//

#include "TestHelper.h"
#include "ftg/sourceloader/BuildDB.h"

#include <experimental/filesystem>
#include <string>

namespace fs = std::experimental::filesystem;
using namespace ftg;

static const std::string TestBinary = "foo";

TEST(TestProjectEntry, TestLoadP) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto ProjectEntryPath =
      (fs::path(BuildDBPath) / "project_entry.json").string();
  auto PE = ProjectEntry::fromJson(ProjectEntryPath);
  ASSERT_EQ(PE->ProjectDir, "/foo");
  ASSERT_EQ(PE->getBCInfo(TestBinary)->BCFile, "/foo/src/foo.bc");
}

TEST(TestProjectEntry, TestLoadWhenNotExistN) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto ProjectEntryPath = (fs::path(BuildDBPath) / "not_exists").string();
  auto PE = ProjectEntry::fromJson(ProjectEntryPath);
  ASSERT_EQ(PE, nullptr);
}

TEST(TestProjectEntry, TestLoadWhenInvalidN) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto ProjectEntryPath =
      (fs::path(BuildDBPath) / "project_entry_invalid.json").string();
  auto PE = ProjectEntry::fromJson(ProjectEntryPath);
  ASSERT_EQ(PE, nullptr);
}
