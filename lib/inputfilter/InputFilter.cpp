#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

InputFilter::InputFilter(std::string FilterName,
                         std::unique_ptr<InputFilter> NextFilter)
    : FilterName(FilterName), NextFilter(std::move(NextFilter)) {}

bool InputFilter::start(const ASTIRNode &Node) const {
  auto Result = check(Node);
  if (!Result && NextFilter)
    return NextFilter->start(Node);
  return Result;
}

bool InputFilter::start(const ASTIRNode &Node,
                        std::set<std::string> &AppliedFilters) const {
  auto Result = check(Node);
  if (Result)
    AppliedFilters.emplace(FilterName);

  if (!NextFilter)
    return !AppliedFilters.empty();
  return NextFilter->start(Node, AppliedFilters);
}

} // namespace ftg
