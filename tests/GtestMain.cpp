#include <gtest/gtest.h>

#define APPROVALS_GOOGLETEST_EXISTING_MAIN
#include "ApprovalTests.hpp"

auto Namer = ApprovalTests::SeparateApprovedAndReceivedDirectoriesNamer::
    useAsDefaultNamer();

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  ApprovalTests::initializeApprovalTestsForGoogleTests();

  return RUN_ALL_TESTS();
}
