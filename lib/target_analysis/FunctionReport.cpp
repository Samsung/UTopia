#include "ftg/targetanalysis/FunctionReport.h"
#include <cassert>

namespace ftg {
FunctionReport::FunctionReport(Function *Definition, bool IsPublic,
                               bool HasCoerced, unsigned BeginArgIndex)
    : Def(Definition), IsPublicAPI(IsPublic), HasCoercedParam(HasCoerced),
      BeginArgIndex(BeginArgIndex) {}

const Function &FunctionReport::getDefinition() const { return *Def; }
bool FunctionReport::isPublicAPI() const { return IsPublicAPI; }

bool FunctionReport::hasCoercedParam() const { return HasCoercedParam; }

ParamReport &FunctionReport::getParam(size_t Index) const {
  assert(Index < Params.size() && "Out of param index in this Function!");
  return *Params[Index];
}

void FunctionReport::addParam(std::shared_ptr<ParamReport> Param) {
  Params.push_back(Param);
}

const std::vector<std::shared_ptr<ParamReport>> &
FunctionReport::getParams() const {
  return Params;
}

unsigned FunctionReport::getBeginArgIndex() const { return BeginArgIndex; }

ParamReport &FunctionReport::getMutableParam(size_t Index) const {
  assert(Index < Params.size() && "Out of param index in this Function!");
  return *Params[Index];
}
} // namespace ftg
