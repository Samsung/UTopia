#ifndef FTG_INPUTFILTER_TYPEUNAVAILABLEFILTER_H
#define FTG_INPUTFILTER_TYPEUNAVAILABLEFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class TypeUnavailableFilter : public InputFilter {
public:
  static const std::string FilterName;
  TypeUnavailableFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_TYPEUNAVAILABLEFILTER_H
