#ifndef FTG_INDCALLSOLVER_TBAASIMPLESOLVERHANDLER_H
#define FTG_INDCALLSOLVER_TBAASIMPLESOLVERHANDLER_H

#include "ftg/indcallsolver/LLVMWalkHandler.h"
#include <llvm/IR/Instruction.h>
#include <map>
#include <set>

namespace ftg {

class TBAASimpleSolverHandler : public LLVMWalkHandler<llvm::Instruction> {
public:
  TBAASimpleSolverHandler() = default;
  TBAASimpleSolverHandler(TBAASimpleSolverHandler &&Handler);
  std::set<const llvm::Function *> get(const llvm::MDNode *Node,
                                       const llvm::Type *Ty) const;
  void handle(const llvm::Instruction &I) override;

private:
  struct MapKey {
    const llvm::MDNode *Node;
    const llvm::Type *Ty;
    MapKey(const llvm::MDNode *Node, const llvm::Type *Ty);
    bool operator<(const MapKey &Key) const;
  };
  std::map<MapKey, std::set<const llvm::Function *>> Map;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_TBAASIMPLESOLVERHANDLER_H
