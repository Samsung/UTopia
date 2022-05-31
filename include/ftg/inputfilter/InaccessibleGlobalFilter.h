#ifndef FTG_INPUTFILTER_INACCESSIBLEGLOBALFILTER_H
#define FTG_INPUTFILTER_INACCESSIBLEGLOBALFILTER_H

#include "ftg/inputfilter/InputFilter.h"

#include "clang/AST/Decl.h"

namespace ftg {

class InaccessibleGlobalFilter : public InputFilter {
public:
  static const std::string FilterName;
  InaccessibleGlobalFilter(std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  virtual bool check(const ASTIRNode &Node) const;

private:
  bool isNonAccessibleInitializer(ASTDefNode &ADN) const;
  bool isNonPublicStaticMember(ASTDefNode &ADN) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_INACCESSIBLEGLOBALFILTER_H
