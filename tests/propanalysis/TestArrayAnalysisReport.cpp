#include "ftg/propanalysis/ArrayAnalysisReport.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(TestArrayAnalysisReport, fromJsonN) {
  ArrayAnalysisReport Report1, Report2;
  Report1.set("Func1", 0, ArrayAnalysisReport::NO_ARRAY);
  ASSERT_TRUE(Report2.fromJson(Report1.toJson()));
  ASSERT_FALSE(Report2.has("Func1", 0));
}
