#ifndef FTG_PROPANALYSIS_ALLOCSIZEANALYZER_H
#define FTG_PROPANALYSIS_ALLOCSIZEANALYZER_H

#include "ftg/propanalysis/AllocSizeAnalysisReport.h"
#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/StackFrame.h"

namespace ftg {

class AllocSizeAnalyzer : public ArgFlowAnalyzer {

public:
  AllocSizeAnalyzer(IndCallSolverMgr *Solver,
                    std::vector<const llvm::Function *> Funcs,
                    const AllocSizeAnalysisReport *PreReport = nullptr);
  AllocSizeAnalyzer(const llvm::Module &M,
                    const AllocSizeAnalysisReport *PreReport = nullptr);

  std::unique_ptr<AnalyzerReport> getReport() override;
  const AllocSizeAnalysisReport &result() const;

private:
  AllocSizeAnalysisReport Report;

  void analyzeProperty(llvm::Argument &A) override;
  void analyzeAlloc(llvm::Argument &A);
  void handleStackFrame(StackFrame &Frame, std::stack<StackFrame> &DefUseChains,
                        std::set<llvm::Value *> &VisitedNodes);
  void handleUser(StackFrame &Frame, llvm::Value &User,
                  std::stack<StackFrame> &DefUseChains,
                  std::set<llvm::Value *> &VisitedNodes);
  bool isAllocFunction(const llvm::Function &F) const;
  bool updateArgFlow(llvm::Argument &A);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
  void updateDefault(const llvm::Module &M);
  void updateDefault(const llvm::Function &F,
                     const std::map<unsigned, bool> &Values);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ALLOCSIZEANALYZER_H
