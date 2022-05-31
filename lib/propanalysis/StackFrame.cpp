#include "ftg/propanalysis/StackFrame.h"

namespace ftg {

StackFrame::StackFrame(const llvm::Value *V, ArgFlow &Flow)
    : Value(const_cast<llvm::Value *>(V)), AnalysisResult(Flow) {}

} // namespace ftg
