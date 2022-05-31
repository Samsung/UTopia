#ifndef FTG_ASTIRMAP_MACROMAPPER_H
#define FTG_ASTIRMAP_MACROMAPPER_H

#include "ftg/astirmap/ASTDefNode.h"
#include "ftg/astirmap/ASTNode.h"
#include "clang/Frontend/ASTUnit.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"

namespace ftg {

class MacroMapper {
public:
  virtual ~MacroMapper() = default;
  virtual void insertMacroNode(clang::Stmt &E, clang::ASTUnit &Unit) = 0;
  virtual ASTNode *getASTNode(llvm::CallBase &CB) = 0;
  virtual ASTDefNode *getASTDefNode(llvm::Instruction &I) = 0;
};

} // namespace ftg

#endif // FTG_ASTIRMAP_MACROMAPPER_H
