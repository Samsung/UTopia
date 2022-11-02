#ifndef FTG_INPUTFILTER_CONSTINTARRAYLENFILTER_H
#define FTG_INPUTFILTER_CONSTINTARRAYLENFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class ConstIntArrayLenFilter : public InputFilter {
public:
  static const std::string FilterName;
  ConstIntArrayLenFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  virtual bool check(const ASTIRNode &Node) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_CONSTINTARRAYLENFILTER_H
