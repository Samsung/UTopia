#ifndef FTG_TCANALYSIS_GOOGLETESTCALLWRITER_H
#define FTG_TCANALYSIS_GOOGLETESTCALLWRITER_H

#include "ftg/tcanalysis/TCCallWriter.h"

namespace ftg {
class GoogleTestCallWriter : public TCCallWriter {
public:
  std::string getTCCall(const Unittest &TC, std::string Indent) const override;

private:
  const std::string FuzzGtestClassName = "AutofuzzTest";
  const std::string FuzzGtestMethodName = "runTest";
  std::string
  getGtestFuzzClassDeclString(const std::string &ParentClassName,
                              const std::vector<FunctionNode> &TestSequence,
                              const std::string &Indent) const;
  std::string getInstanceNameFromClass(const std::string &ClassName) const;
};
} // namespace ftg

#endif // FTG_TCANALYSIS_GOOGLETESTCALLWRITER_H
