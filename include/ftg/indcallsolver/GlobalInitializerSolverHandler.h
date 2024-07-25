#ifndef FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVERHANDLER_H
#define FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVERHANDLER_H

#include "ftg/indcallsolver/LLVMWalkHandler.h"
#include <set>
#include <map>

namespace ftg {

class GlobalInitializerSolverHandler
    : public LLVMWalkHandler<llvm::GlobalVariable> {
public:
  GlobalInitializerSolverHandler() = default;
  GlobalInitializerSolverHandler(GlobalInitializerSolverHandler &&Handler);
  void handle(const llvm::GlobalVariable &GV) override;
  std::set<const llvm::Function *> get(const llvm::Type *Ty,
                                       unsigned Idx) const;

private:
  struct MapKey {
    const llvm::Type *Ty;
    unsigned Idx;
    MapKey(const llvm::Type *Ty, unsigned Idx);
    bool operator<(const MapKey &Key) const;
  };
  std::map<MapKey, std::set<const llvm::Function *>> Map;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_GLOBALINITIALIZERSOLVERHANDLER_H
