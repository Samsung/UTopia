#include "ftg/propanalysis/PropAnalysisCommon.h"

using namespace llvm;

namespace ftg {

ICmpInst *getLoopExitCond(Loop &L) {
  BranchInst *BI = nullptr;
  if (L.isRotatedForm()) {
    auto LL = L.getLoopLatch();
    assert(LL && "Unexpected Program State");

    BI = dyn_cast<BranchInst>(LL->getTerminator());
  } else {
    auto Header = L.getHeader();
    assert(Header && "Unexpected Program State");

    BI = dyn_cast<BranchInst>(Header->getTerminator());
  }

  if (BI && BI->isConditional()) {
    assert((BI->getSuccessor(0) != BI->getSuccessor(1)) &&
           "Both outgoing branches should not target same header!");
    return dyn_cast<ICmpInst>(BI->getCondition());
  }

  return nullptr;
}

} // namespace ftg
