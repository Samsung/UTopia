#include "ftg/utils/FileUtil.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(GetParentDirectory, P) {

  EXPECT_EQ(util::getParentPath("/A/B"), "/A");
  EXPECT_EQ(util::getParentPath("A/B"), "A");
  EXPECT_EQ(util::getParentPath("A"), "");
}

TEST(GetParentDirectory, N) {

  EXPECT_EQ(util::getParentPath("/"), "");
  EXPECT_EQ(util::getParentPath("A/B/"), "A/B");
}

TEST(TestFileUtil, RebasePathP) {
  ASSERT_EQ(util::rebasePath("/a/b", "/a", "/c"), "/c/b");
}

TEST(TestFileUtilDeathTest, RebasePathInvalidN) {
  ASSERT_DEATH(util::rebasePath("/a/b", "/c", "/a"), "");
}
