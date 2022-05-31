#ifndef FTG_ROOTDEFANALYSIS_RDNODE_H
#define FTG_ROOTDEFANALYSIS_RDNODE_H

#include "RDBasicTypes.h"
#include "RDTarget.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include <set>

namespace ftg {

class RDNode {
public:
  RDNode(int OpIdx, llvm::Instruction &I, const RDNode *Before = nullptr);
  RDNode(llvm::Value &V, llvm::Instruction &I, const RDNode *Before = nullptr);
  RDNode(const RDTarget &Target, llvm::Instruction &I,
         const RDNode *Before = nullptr);
  RDNode(const RDNode &Src);

  void setIdx(int Idx);
  void setTarget(llvm::Value &V);
  void setLocation(llvm::Instruction &I);
  void setVisit(
      const std::map<llvm::Function *, std::set<llvm::Instruction *>> &Visit);
  void setFirstUses(const std::set<RDArgIndex> &FirstUses);
  void setFirstUse(llvm::CallBase &CB, int ArgNo);
  void setForcedDef();
  void timeout();

  void visit(llvm::Instruction &I);
  void addFirstUse(llvm::CallBase &CB, int ArgNo);
  void addFirstUse(const RDArgIndex &Src);
  void addFirstUses(const std::set<RDArgIndex> &FirstUses);

  void cancelVisit(llvm::Function &F);
  void clearVisit();
  void eraseEmptyFirstUse();
  void clearFirstUses();

  int getIdx() const;
  const RDTarget &getTarget() const;
  llvm::Instruction &getLocation();
  const std::map<llvm::Function *, std::set<llvm::Instruction *>> &
  getVisit() const;
  const std::set<RDArgIndex> &getFirstUses() const;
  std::pair<llvm::Value *, int> getDefinition() const;

  bool isVisit(llvm::Instruction &I) const;
  bool isRootDefinition() const;
  bool isMemory() const;
  bool isTimeout() const;

  RDNode &operator=(const RDNode &RHS);
  bool operator<(const RDNode &Src) const;
  bool operator==(const RDNode &Src) const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const RDNode &Src);

private:
  int Idx;
  std::shared_ptr<RDTarget> Target;
  llvm::Instruction *Location;
  std::map<llvm::Function *, std::set<llvm::Instruction *>> Visit;
  std::set<RDArgIndex> FirstUses;
  bool ForcedDef;
  bool Timeout;

  int getIdx(llvm::Value &V, llvm::Instruction &I) const;
  int getIdx(const RDTarget &T, llvm::Instruction &I) const;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDNODE_H
