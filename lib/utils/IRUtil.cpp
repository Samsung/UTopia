#include "ftg/utils/IRUtil.h"

#include "ftg/utils/ASTUtil.h"

#include "clang/AST/Decl.h"
#include "llvm/IR/Module.h"

namespace ftg {

namespace util {
const llvm::Function *getIRFunction(const llvm::Module &M,
                                    const clang::NamedDecl *ND) {
  std::string MangledDeclName =
      util::getMangledName(const_cast<clang::NamedDecl *>(ND));
  return M.getFunction(MangledDeclName);
}

} // namespace util
} // namespace ftg
