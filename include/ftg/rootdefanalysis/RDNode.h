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

  void copyVisit(const RDNode &Src);
  void setIdx(int Idx);
  void setTarget(llvm::Value &V);
  void setLocation(llvm::Instruction &I);
  void setFirstUses(const std::set<RDArgIndex> &FirstUses);
  void setFirstUse(llvm::CallBase &CB, int ArgNo);
  void setForcedDef();
  void setStopTracing(bool Flag);
  void timeout();

  void visit(const RDTarget &T, const llvm::Instruction &I);
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
  const std::set<RDArgIndex> &getFirstUses() const;
  std::pair<llvm::Value *, int> getDefinition() const;

  bool isVisit(const RDTarget &T, const llvm::Instruction &I) const;
  bool isVisit(const llvm::Function &F) const;
  bool isRootDefinition() const;
  bool isMemory() const;
  bool isTimeout() const;
  bool isStopTracing() const;

  RDNode &operator=(const RDNode &RHS);
  bool operator<(const RDNode &Src) const;
  bool operator==(const RDNode &Src) const;
  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &O, const RDNode &Src);

private:
  struct RDVisitNode {
    const std::shared_ptr<const RDTarget> T;
    const llvm::Instruction &I;
    RDVisitNode(const RDTarget &T, const llvm::Instruction &I);
    bool operator<(const RDVisitNode &Rhs) const;
  };
  int Idx;
  std::shared_ptr<RDTarget> Target;
  llvm::Instruction *Location;
  std::map<const llvm::Function *, std::set<RDVisitNode>> Visit;
  std::set<RDArgIndex> FirstUses;
  bool ForcedDef;
  bool StopTracing;
  bool Timeout;

  int getIdx(llvm::Value &V, llvm::Instruction &I) const;
  int getIdx(const RDTarget &T, llvm::Instruction &I) const;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDNODE_H
