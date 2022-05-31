#ifndef FTG_TARGETANALYSIS_FUNCTIONREPORT_H
#define FTG_TARGETANALYSIS_FUNCTIONREPORT_H

#include <cstddef>
#include <memory>
#include <vector>

namespace ftg {

class Function;

class FunctionReport;
class ParamReport;

class FunctionReport {
public:
  FunctionReport(Function *Definition, bool IsPublic = false,
                 bool HasCoerced = false, unsigned BeginArgIndex = 0);
  const Function &getDefinition() const;
  bool isPublicAPI() const;
  bool hasCoercedParam() const;
  ParamReport &getParam(size_t Index) const;
  void addParam(std::shared_ptr<ParamReport> Param);
  const std::vector<std::shared_ptr<ParamReport>> &getParams() const;
  unsigned getBeginArgIndex() const;

protected:
  friend class TargetLibAnalyzer;
  ParamReport &getMutableParam(size_t Index) const;

private:
  Function *Def;
  bool IsPublicAPI;
  bool HasCoercedParam;
  unsigned BeginArgIndex;
  std::vector<std::shared_ptr<ParamReport>> Params;
};
} // namespace ftg

#endif // FTG_TARGETANALYSIS_FUNCTIONREPORT_H
