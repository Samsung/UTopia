//===-- TestCompileDB.cpp - Unit tests for CompileDB ----------------------===//

#include "TestHelper.h"
#include "ftg/sourceloader/BuildDB.h"

#include <experimental/filesystem>
#include <string>

namespace fs = std::experimental::filesystem;
using namespace ftg;

static const std::string TestBinary = "foo";

TEST(TestCompileDB, TestLoadP) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto CompileDBPath =
      (fs::path(BuildDBPath) / ("compiles_" + TestBinary + ".json")).string();
  auto DB = CompileDB::fromJson(CompileDBPath);
  ASSERT_EQ(DB->Compiles[0].ASTFile, "/foo/src/src1.c.ast");
  ASSERT_EQ(DB->Compiles[1].ASTFile, "/foo/src/src2.c.ast");
}

TEST(TestCompileDB, TestLoadWhenNotExistN) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto CompileDBPath = (fs::path(BuildDBPath) / "not_exists").string();
  auto DB = CompileDB::fromJson(CompileDBPath);
  ASSERT_EQ(DB, nullptr);
}

TEST(TestCompileDB, TestLoadWhenInvalidFormatN) {
  auto BuildDBPath = getProjectBaseDirPath("test_builddb");
  auto CompileDBPath =
      (fs::path(BuildDBPath) / "compiles_invalid.json").string();
  auto DB = CompileDB::fromJson(CompileDBPath);
  ASSERT_EQ(DB, nullptr);
}
