#ifndef FTG_INDCALLSOLVER_INDCALLSOLVERIMPL_H
#define FTG_INDCALLSOLVER_INDCALLSOLVERIMPL_H

#include "IndCallSolver.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include <set>

namespace ftg {

class IndCallSolverImpl : public IndCallSolver {

public:
  void solve(llvm::Module &M) override;
  llvm::Function *getCalledFunction(llvm::CallBase &CB) override;

private:
  struct DevirtCallSite {
    // Called Value
    llvm::Value *CV;
    // Offset at VTable
    uint64_t VTOffset;
  };

  std::map<llvm::Type *, std::map<unsigned, std::set<llvm::Function *>>>
      GlobalMap;
  std::map<llvm::MDNode *, std::map<llvm::Type *, std::set<llvm::Function *>>>
      InstMap;
  std::map<llvm::Value *, std::set<llvm::Function *>> VTMap;

  void buildGlobalMap(llvm::Module &M);
  void buildInstMap(llvm::Module &M);
  void buildVTMap(llvm::Module &M);
  llvm::MDNode *findTBAA(llvm::Instruction &I) const;
  void
  findDeVirtualizableCalls(llvm::SmallVectorImpl<DevirtCallSite> &DevirtCalls,
                           llvm::Value &V, uint64_t Offset,
                           llvm::Module &M) const;
  void findCalledValue(llvm::SmallVectorImpl<DevirtCallSite> &DevirtCalls,
                       llvm::Value &V, uint64_t Offset) const;
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_INDCALLSOLVERIMPL_H
