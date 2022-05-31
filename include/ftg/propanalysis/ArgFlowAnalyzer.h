#ifndef FTG_PROPANALYSIS_ARGFLOWANALYZER
#define FTG_PROPANALYSIS_ARGFLOWANALYZER

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/propanalysis/ArgFlow.h"
#include "ftg/propanalysis/PropAnalyzer.h"

namespace ftg {

class ArgFlowAnalyzer : public PropAnalyzer {

public:
  ArgFlowAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                  const std::vector<const llvm::Function *> &Funcs);
  void analyze(const llvm::Argument &A) override;
  const std::map<llvm::Argument *, std::shared_ptr<ArgFlow>>
  getArgFlowMap() const;

protected:
  std::shared_ptr<IndCallSolver> Solver;
  std::map<llvm::Argument *, std::shared_ptr<ArgFlow>> ArgFlowMap;

  void analyze(const std::vector<const llvm::Function *> &Funcs);
  virtual void analyzeProperty(llvm::Argument &A) = 0;
  llvm::ArrayType *getAsArrayType(llvm::Value &V) const;
  llvm::StructType *getAsStructType(llvm::Value &V) const;
  llvm::Function *getCalledFunction(llvm::CallBase &CB) const;
  ArgFlow &getOrCreateArgFlow(llvm::Argument &A);
  bool mayThrow(const llvm::BasicBlock &BB) const;
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ARGFLOWANALYZER
