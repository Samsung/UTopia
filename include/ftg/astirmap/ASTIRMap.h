#ifndef FTG_ASTIRMAP_ASTIRMAP_H
#define FTG_ASTIRMAP_ASTIRMAP_H

#include "ftg/astirmap/ASTDefNode.h"
#include "clang/AST/DeclCXX.h"
#include "llvm/IR/InstrTypes.h"

namespace ftg {

class ASTIRMap {

public:
  virtual ~ASTIRMap() = default;
  virtual ASTDefNode *getASTDefNode(const llvm::Value &V, int OIdx = -1) = 0;
  virtual bool hasDiffNumArgs(const llvm::CallBase &CB) const = 0;
  virtual unsigned getDiffNumArgs(const llvm::CallBase &CB) const = 0;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_ASTIRMAP_H
