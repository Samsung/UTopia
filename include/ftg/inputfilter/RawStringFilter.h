#ifndef FTG_INPUTFILTER_RAWSTRINGFILTER_H
#define FTG_INPUTFILTER_RAWSTRINGFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class RawStringFilter : public InputFilter {
public:
  static std::string FilterName;
  RawStringFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_RAWSTRINGFILTER_H
