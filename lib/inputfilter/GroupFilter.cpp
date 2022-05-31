#include "ftg/inputfilter/GroupFilter.h"

namespace ftg {

GroupFilter::GroupFilter(std::string FilterName,
                         std::unique_ptr<GroupFilter> NextFilter)
    : FilterName(FilterName), NextFilter(std::move(NextFilter)) {}

void GroupFilter::start(
    std::map<unsigned, std::shared_ptr<Definition>> &DefMap) {
  check(DefMap);
  if (NextFilter)
    NextFilter->start(DefMap);
}

} // namespace ftg
