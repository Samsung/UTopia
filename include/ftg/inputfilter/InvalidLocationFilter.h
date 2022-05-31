#ifndef FTG_INPUTFILTER_INVALIDLOCATIONFILTER_H
#define FTG_INPUTFILTER_INVALIDLOCATIONFILTER_H

#include "ftg/inputfilter/InputFilter.h"

namespace ftg {

class InvalidLocationFilter : public InputFilter {
public:
  static const std::string FilterName;
  InvalidLocationFilter(std::string BaseDir,
                        std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  bool check(const ASTIRNode &Node) const override;

private:
  std::string BaseDir;
  std::tuple<std::string, unsigned, unsigned>
  getLoc(const ASTDefNode &ADN) const;
  bool isHeaderFile(const std::string &Name) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_INVALIDLOCATIONFILTER_H
