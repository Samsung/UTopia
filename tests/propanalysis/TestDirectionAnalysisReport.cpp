#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(TestDirectionAnalysisReport, fromJsonN) {
  DirectionAnalysisReport Report1, Report2;
  Report1.set("Func1", 0, Dir_NoOp);
  ASSERT_TRUE(Report2.fromJson(Report1.toJson()));
  ASSERT_FALSE(Report2.has("Func1", 0));
}
