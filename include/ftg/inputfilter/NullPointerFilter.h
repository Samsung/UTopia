#ifndef FTG_INPUTFILTER_NULLPOINTERFILTER_H
#define FTG_INPUTFILTER_NULLPOINTERFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class NullPointerFilter : public InputFilter {
public:
  static const std::string FilterName;
  NullPointerFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_NULLPOINTERFILTER_H
