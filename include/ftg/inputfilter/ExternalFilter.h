#ifndef FTG_INPUTFILTER_EXTERNALFILTER_H
#define FTG_INPUTFILTER_EXTERNALFILTER_H

#include "ftg/inputfilter/InputFilter.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"

namespace ftg {

class ExternalFilter : public InputFilter {
public:
  static const std::string FilterName;
  ExternalFilter(const DirectionAnalysisReport &Report,
                 std::unique_ptr<InputFilter> NextFilter = nullptr);

protected:
  const DirectionAnalysisReport &DirectionReport;
  bool check(const ASTIRNode &Node) const override;

private:
  bool isCallReturn(const ASTDefNode &Node) const;
};

} // namespace ftg

#endif // FTG_INPUTFILTER_EXTERNALFILTER_H
