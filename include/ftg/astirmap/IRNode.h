#ifndef FTG_ASTIRMAP_IRNODE_H
#define FTG_ASTIRMAP_IRNODE_H

#include "ftg/astirmap/LocIndex.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

namespace ftg {

class IRNode {
public:
  IRNode(const llvm::Value &V);
  std::string getName() const;
  const LocIndex &getIndex() const;
  bool operator<(const IRNode &Rhs) const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const IRNode &Loc);

private:
  LocIndex Index;
  std::string Name;

  std::string getFullPath(const llvm::DebugLoc &Loc) const;
  std::string getFullPath(const llvm::DIGlobalVariable &G) const;
  std::string getFullPath(const llvm::DIFile &F) const;
  void setIndex(const llvm::AllocaInst &I);
  void setIndex(const llvm::GlobalValue &G);
  void setIndex(const llvm::Instruction &I);
};

} // namespace ftg

#endif // FTG_ASTIRMAP_IRNODE_H
