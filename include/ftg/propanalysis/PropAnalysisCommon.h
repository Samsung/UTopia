#ifndef FTG_PROPANALYSIS_PROPANALYSISCOMMON_H
#define FTG_PROPANALYSIS_PROPANALYSISCOMMON_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"

namespace ftg {

llvm::ICmpInst *getLoopExitCond(llvm::Loop &L);

} // namespace ftg

#endif // FTG_PROPANALYSIS_PROPANALYSISCOMMON_H
