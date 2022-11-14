#ifndef FTG_INDCALLSOLVER_INDCALLSOLVERUTIL_HPP
#define FTG_INDCALLSOLVER_INDCALLSOLVERUTIL_HPP

#include <llvm/IR/Instructions.h>

namespace ftg {

static inline const llvm::MDNode *getTBAA(const llvm::Instruction *I) {
  const auto *ExprI = I;
  while (ExprI && (llvm::isa<llvm::LoadInst>(ExprI) ||
                   llvm::isa<llvm::GetElementPtrInst>(ExprI))) {
    const auto *TBAA = ExprI->getMetadata(llvm::LLVMContext::MD_tbaa);
    if (TBAA)
      return TBAA;

    const auto *Op = ExprI->getOperand(0);
    if (!Op)
      break;

    ExprI = llvm::dyn_cast_or_null<llvm::Instruction>(Op->stripPointerCasts());
  }
  return nullptr;
}

} // namespace ftg

#endif // FTG_INDCALLSOLVER_INDCALLSOLVERUTIL_HPP
