#include "ftg/tcanalysis/TCTCallWriter.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

std::string TCTCallWriter::getTCCall(const Unittest &TC,
                                     std::string Indent) const {
  std::string PreTestFunc, TestMainFunc, PostTestFunc;
  bool PreAction = true;
  for (const auto &TestFunc : TC.getTestSequence()) {
    if (TestFunc.getName().empty())
      continue;

    if (TestFunc.isTestBody()) {
      TestMainFunc = TestFunc.getName() + "();";
      PreAction = false;
      continue;
    }

    if (PreAction) {
      PreTestFunc = TestFunc.getName() + "();";
      continue;
    }

    PostTestFunc = TestFunc.getName() + "();";
  }

  std::string Result;
  if (!PreTestFunc.empty())
    Result += Indent + PreTestFunc + "\n";
  if (!TestMainFunc.empty())
    Result += Indent + TestMainFunc + "\n";
  if (!PostTestFunc.empty())
    Result += Indent + PostTestFunc + "\n";
  return Result;
}
