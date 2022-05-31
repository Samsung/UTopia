#ifndef FTG_INPUTFILTER_INPUTFILTER_H
#define FTG_INPUTFILTER_INPUTFILTER_H

#include "ftg/inputanalysis/ASTIRNode.h"
#include <memory>
#include <string>

namespace ftg {

class InputFilter {
public:
  virtual ~InputFilter() = default;
  InputFilter(std::string FilterName, std::unique_ptr<InputFilter> NextFilter);
  bool start(const ASTIRNode &Node) const;
  bool start(const ASTIRNode &Node,
             std::set<std::string> &AppliedFilters) const;

protected:
  virtual bool check(const ASTIRNode &Node) const = 0;

private:
  std::string FilterName;
  std::unique_ptr<InputFilter> NextFilter;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_INPUTFILTER_H
