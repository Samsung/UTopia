#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/inputanalysis/DefMapGenerator.h"
#include "ftg/inputfilter/ArrayGroupFilter.h"
#include "ftg/inputfilter/CompileConstantFilter.h"
#include "ftg/inputfilter/ConstIntArrayLenFilter.h"
#include "ftg/inputfilter/ExternalFilter.h"
#include "ftg/inputfilter/InaccessibleGlobalFilter.h"
#include "ftg/inputfilter/InvalidLocationFilter.h"
#include "ftg/inputfilter/NullPointerFilter.h"
#include "ftg/inputfilter/RawStringFilter.h"
#include "ftg/inputfilter/TypeUnavailableFilter.h"
#include "ftg/inputfilter/UnsupportTypeFilter.h"
#include "ftg/propanalysis/AllocAnalyzer.h"

using namespace ftg;

class TestInputAnalysisBase : public testing::Test {
protected:
  std::unique_ptr<UTLoader> Loader;

  bool load(std::string SrcDir, std::string CodePath) {
    std::vector<std::string> CodePaths = { CodePath };
    return load(SrcDir, CodePaths);
  }

  bool load(std::string SrcDir, std::vector<std::string> CodePaths) {
    auto CH = TestHelperFactory().createCompileHelper(
        SrcDir, CodePaths, "-O0 -g -w", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    Loader =
        std::make_unique<UTLoader>(CH, nullptr, std::vector<std::string>());
    if (!Loader)
      return false;

    return true;
  }
};
