#ifndef FTG_ROOTDEFANALYSIS_RDSPACE_H
#define FTG_ROOTDEFANALYSIS_RDSPACE_H

#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/PassManager.h"
#include <map>

namespace ftg {

class RDSpace {
public:
  RDSpace();
  void build(const std::vector<llvm::Function *> &Funcs);

  // Interface method candidates.
  std::set<llvm::Instruction *> next(llvm::Instruction &I);
  std::set<llvm::Instruction *> nextInCallee(const llvm::Function &F,
                                             bool Memory = false);
  std::set<llvm::Instruction *> nextInCaller(const llvm::Function &F);
  std::set<llvm::Instruction *> nextInLink(const llvm::Function &F);

  bool isEntryFunction(llvm::Function &F) const;
  std::set<llvm::Instruction *> getAliases(llvm::Instruction &I) const;
  bool dominates(llvm::Instruction &Dominator, llvm::Instruction &Dominatee);

private:
  llvm::FunctionAnalysisManager FAM;
  std::vector<llvm::Function *> EntryFuncs;
  std::set<llvm::Function *> BuiltFuncs;
  std::map<llvm::Function *, llvm::Function *> LinkMap;
  std::map<llvm::Function *, std::set<llvm::CallBase *>> CallerMap;
  std::map<llvm::Function *, std::set<llvm::ReturnInst *>> ReturnMap;
  std::map<llvm::Instruction *, std::set<llvm::Instruction *>> AliasMap;
  std::map<llvm::Function *, std::set<llvm::Instruction *>> LeafMemMap;

  std::set<llvm::Function *> buildCallerMap(llvm::Function &);
  void build(llvm::Function &);
  std::set<llvm::Instruction *> getLastMemInsts(llvm::Function &,
                                                llvm::Instruction &,
                                                std::set<llvm::BasicBlock *> &);
  template <typename KTy, typename VTy, typename MTy>
  void updateMap(MTy &, KTy, VTy);

  std::set<llvm::MemoryAccess *> next(llvm::MemoryAccess &MA);
  std::set<llvm::Instruction *>
  getInstructions(llvm::MemoryAccess &MA, bool WithNull = false,
                  std::set<llvm::MemoryAccess *> Visit = {});
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDSPACE_H
