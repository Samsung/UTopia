#include "ftg/utils/IRUtil.h"
#include "ftg/utils/LLVMUtil.h"

#include "clang/AST/Decl.h"
#include "llvm/IR/Module.h"

namespace ftg {

namespace util {
const llvm::Function *getIRFunction(const llvm::Module &M,
                                    const clang::NamedDecl *ND) {
  if (!ND)
    return nullptr;

  // NOTE: Unexpected when multiple functions are matched by one clang
  // NamedDecl.
  for (const auto &MangledName : util::getMangledNames(*ND)) {
    const auto *F = M.getFunction(MangledName);
    if (F)
      return F;
  }
  return nullptr;
}

} // namespace util
} // namespace ftg
