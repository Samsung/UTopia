#ifndef FTG_INDCALLSOLVER_TBAAVIRTSOLVERHANDLER_H
#define FTG_INDCALLSOLVER_TBAAVIRTSOLVERHANDLER_H

#include "ftg/indcallsolver/LLVMWalkHandler.h"
#include <llvm/IR/Instruction.h>
#include <map>
#include <set>

namespace ftg {

class TBAAVirtSolverHandler : public LLVMWalkHandler<llvm::Instruction> {
public:
  TBAAVirtSolverHandler() = default;
  TBAAVirtSolverHandler(TBAAVirtSolverHandler &&Handler);
  std::set<const llvm::Constant *> get(const llvm::MDNode *Node) const;
  void handle(const llvm::Instruction &I) override;

private:
  std::map<const llvm::MDNode *, std::set<const llvm::Constant *>> Map;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TBAAVIRTSOLVERHANDLER_H
