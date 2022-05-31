#ifndef FTG_INPUTFILTER_UNSUPPORTTYPEFILTER_H
#define FTG_INPUTFILTER_UNSUPPORTTYPEFILTER_H

#include "ftg/inputfilter/InputFilter.h"
#include "ftg/type/Type.h"

namespace ftg {

class UnsupportTypeFilter : public InputFilter {
public:
  static const std::string FilterName;
  UnsupportTypeFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_UNSUPPORTTYPEFILTER_H
