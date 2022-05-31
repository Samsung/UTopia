#ifndef FTG_PROPANALYSIS_ALLOCANALYZER_H
#define FTG_PROPANALYSIS_ALLOCANALYZER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/propanalysis/AllocAnalysisReport.h"
#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/StackFrame.h"

namespace ftg {

class AllocAnalyzer : public ArgFlowAnalyzer {

public:
  AllocAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                std::vector<const llvm::Function *> Funcs,
                const AllocAnalysisReport *PreReport = nullptr);
  AllocAnalyzer(const llvm::Module &M,
                const AllocAnalysisReport *PreReport = nullptr);

  std::unique_ptr<AnalyzerReport> getReport() override;
  const AllocAnalysisReport &result() const;

private:
  AllocAnalysisReport Report;

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
                     const std::map<unsigned, ArgAlloc> &Values);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_ALLOCANALYZER_H
