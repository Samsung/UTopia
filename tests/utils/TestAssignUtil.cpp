#include "ftg/utils/AssignUtil.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(TestAssignUtil, StripConstExprP) {
  EXPECT_EQ("int", util::stripConstExpr("const int"));
  EXPECT_EQ("int", util::stripConstExpr("constexpr int"));
  EXPECT_EQ("int *", util::stripConstExpr("int const*"));
  EXPECT_EQ("int *", util::stripConstExpr("int *const"));
  EXPECT_EQ("int", util::stripConstExpr("const int "));
  EXPECT_EQ("int", util::stripConstExpr("const  int"));
}

TEST(TestAssignUtil, StripConstExprN) {
  EXPECT_EQ("int", util::stripConstExpr("int"));
}
