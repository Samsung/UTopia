#ifndef FTG_PROPANALYSIS_STACKFRAME_H
#define FTG_PROPANALYSIS_STACKFRAME_H

#include "ftg/propanalysis/ArgFlow.h"

namespace ftg {

class StackFrame {

public:
  llvm::Value *Value;
  ArgFlow &AnalysisResult;
  unsigned Depth = 0;

  StackFrame(const llvm::Value *V, ArgFlow &argFlow);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_STACKFRAME_H
