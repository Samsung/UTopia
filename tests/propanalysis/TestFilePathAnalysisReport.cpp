#include "TestHelper.h"
#include "ftg/propanalysis/FilePathAnalysisReport.h"

using namespace ftg;

TEST(TestFilePathAnalysisReport, fromJsonN) {
  FilePathAnalysisReport Report1, Report2;
  Report1.set("Func1", 0, false);
  ASSERT_TRUE(Report2.fromJson(Report1.toJson()));
  ASSERT_FALSE(Report2.has("Func1", 0));
}
