#ifndef FTG_INPUTFILTER_COMPILECONSTANTFILTER_H
#define FTG_INPUTFILTER_COMPILECONSTANTFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class CompileConstantFilter : public InputFilter {
public:
  static const std::string FilterName;
  CompileConstantFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;

private:
  bool isMacroFunctionAssigned(const ASTDefNode &Node) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_COMPILECONSTANTFILTER_H
