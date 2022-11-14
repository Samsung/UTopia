#include "ftg/propanalysis/AllocSizeAnalysisReport.h"
#include <gtest/gtest.h>

using namespace ftg;

TEST(TestAllocSizeAnalysisReport, fromJsonN) {
  AllocSizeAnalysisReport Report1, Report2;
  Report1.set("Func1", 0, false);
  ASSERT_TRUE(Report2.fromJson(Report1.toJson()));
  ASSERT_FALSE(Report2.has("Func1", 0));
}
