#ifndef FTG_UTILS_IRUTIL_H
#define FTG_UTILS_IRUTIL_H

#include "ftg/utils/ASTUtil.h"

#include "clang/AST/Decl.h"
#include "llvm/IR/Module.h"

namespace ftg {

namespace util {
const llvm::Function *getIRFunction(const llvm::Module &M,
                                    const clang::NamedDecl *ND);

} // namespace util
} // namespace ftg

#endif // FTG_UTILS_IRUTIL_H
