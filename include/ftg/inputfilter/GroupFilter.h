#ifndef FTG_INPUTFILTER_GROUPFILTER_H
#define FTG_INPUTFILTER_GROUPFILTER_H

#include "ftg/inputanalysis/ASTIRNode.h"
#include "ftg/inputanalysis/Definition.h"
#include <memory>
#include <string>

namespace ftg {

class GroupFilter {
public:
  virtual ~GroupFilter() = default;
  GroupFilter(std::string FilterName, std::unique_ptr<GroupFilter> NextFilter);
  void start(std::map<unsigned, std::shared_ptr<Definition>> &DefMap);

protected:
  virtual void
  check(std::map<unsigned, std::shared_ptr<Definition>> &DefMap) const = 0;

private:
  std::string FilterName;
  std::unique_ptr<GroupFilter> NextFilter;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_INPUTFILTER_H
