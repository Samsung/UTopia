#ifndef FTG_ROOTDEFANALYSIS_RDANALYZER_H
#define FTG_ROOTDEFANALYSIS_RDANALYZER_H

#include "RDCache.h"
#include "RDExtension.h"
#include "RDSpace.h"
#include "RootDefAnalyzer.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include <time.h>

namespace ftg {

class RDAnalyzer : public RootDefAnalyzer {
public:
  RDAnalyzer(unsigned Timeout = 0, RDExtension *Extension = nullptr);
  void setSearchSpace(const std::vector<llvm::Function *> &Functions) override;
  std::set<RDNode> getRootDefinitions(const llvm::Use &U) override;

private:
  enum PROPTYPE { PROPTYPE_NONE = 0, PROPTYPE_PART, PROPTYPE_INCL };
  RDSpace Space;
  RDCache Cache;
  RDExtension Extension;
  unsigned Timeout;
  time_t Start;

  std::set<RDNode> findRootDefinitions(llvm::Use &U);
  std::set<RDNode> traverseFunction(RDNode &Node, llvm::Function &F,
                                    bool TraceCaller, bool TraceCallee);
  std::set<RDNode> next(RDNode &Node);

  std::set<RDNode> getPrevs(RDNode &);
  std::set<RDNode> getPrevsMemoryBase(RDNode &);
  std::set<RDNode> getPrevsRegBase(RDNode &);
  std::set<RDNode> trace(RDNode &);
  std::set<RDNode> traceCaller(RDNode &, llvm::Function &);
  std::set<RDNode> traceLink(RDNode &, llvm::Function &);
  std::set<RDNode> pass(RDNode &);
  std::set<RDNode> handleRegister(RDNode &Node, llvm::CallBase &CB);
  std::set<RDNode> handleRegisterInternal(RDNode &Node, llvm::CallBase &CB);
  std::set<RDNode> handleStoreInst(RDNode &, llvm::StoreInst &);
  std::set<RDNode> handleMemoryCall(RDNode &, llvm::CallBase &);
  std::set<RDNode> handleExternalCall(RDNode &Node, llvm::CallBase &CB);
  std::set<RDNode> handleMemoryIntrinsicCall(RDNode &Node, llvm::CallBase &CB);
  bool isOutParam(const llvm::CallBase &, size_t) const;
  std::set<size_t> getOutParamIndices(llvm::CallBase &) const;
  std::set<unsigned> getNonOutParamIndices(const llvm::CallBase &) const;
  bool isForcedDef(RDNode &Node) const;
  bool terminates(RDNode &Node) const;
  bool isMemoryIntrinsic(llvm::CallBase &CB) const;
  bool isTargetMethodInvocation(const RDNode &Node, const llvm::CallBase &CB);
  bool isNonStaticMethodInvocation(const llvm::CallBase &CB) const;

  PROPTYPE isPropagated(const RDTarget &Target, llvm::Value &Src);
  void mergeRDNodes(std::set<RDNode> &Dst, std::set<RDNode> Src) const;
  void mergeRDNode(std::set<RDNode> &Dst, const RDNode &Src) const;
};

} // namespace ftg

#endif // FTG_ROOTDEFANALYSIS_RDANALYZER_H
