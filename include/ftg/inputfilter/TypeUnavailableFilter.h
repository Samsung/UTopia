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

private:
  // TODO: DeclTypeInfo should be vector to include all redecls of the same
  // variable declaration. Currently, this code will not give proper result
  // for the case.
  bool hasConstRedecls(const ASTDefNode &ADN) const;
  bool isUndefinedType(const ASTDefNode &ADN) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_TYPEUNAVAILABLEFILTER_H
