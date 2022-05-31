//===-- TestFuzzStatus.cpp - Unit tests for FuzzStatus --------------------===//

#include "ftg/generation/FuzzStatus.h"
#include "gtest/gtest.h"

using namespace ftg;

TEST(TestFuzzStatus, TestEqualityP) {
  FuzzStatus Status1;
  FuzzStatus Status2(FUZZABLE_SRC_GENERATED);
  FuzzStatus Status3 = FUZZABLE_SRC_GENERATED;
  ASSERT_TRUE(Status1 == UNINITIALIZED);
  ASSERT_TRUE(Status1 != FUZZABLE_SRC_GENERATED);
  ASSERT_TRUE(Status1 != Status2);
  ASSERT_TRUE(Status2 == Status3);
}
