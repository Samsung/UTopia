//===-- TestLocIndex.cpp - Unit tests for LocIndex ------------------------===//

#include "TestHelper.h"
#include "ftg/astirmap/LocIndex.h"

using namespace ftg;

using TestLocIndex = TestBase;

TEST_F(TestLocIndex, InstructionTestP) {
  const std::string Code = R"(
  extern "C" {
    void API();
    void test() {
      API();
    }
  }
  )";
  loadCPP(Code);

  auto *I = IRAH->getInstruction("test", 0, 0);
  auto ILoc = LocIndex::of(*I);
  ASSERT_EQ(5, ILoc.getLine());
  ASSERT_EQ(7, ILoc.getColumn());
  ASSERT_EQ("/tmp/default_test.cpp", ILoc.getPath());
}

TEST_F(TestLocIndex, AllocaInstTestP) {
  const std::string Code = R"(
  extern "C" {
    void withAlloc() {
      int Arr[10] = {0,};
    }
  }
  )";
  loadCPP(Code);

  auto *AI = IRAH->getInstruction("withAlloc", 0, 0);
  auto AILoc = LocIndex::of(*AI);
  ASSERT_EQ(4, AILoc.getLine());
  ASSERT_EQ(11, AILoc.getColumn());
  ASSERT_EQ("/tmp/default_test.cpp", AILoc.getPath());
}
