#ifndef FTG_PROPANALYSIS_LOOPANALYZER_H
#define FTG_PROPANALYSIS_LOOPANALYZER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/LoopAnalysisReport.h"
#include "ftg/propanalysis/StackFrame.h"
#include "llvm/IR/PassManager.h"

namespace ftg {

class LoopAnalyzer : public ArgFlowAnalyzer {
public:
  LoopAnalyzer(std::shared_ptr<IndCallSolver> Solver,
               std::vector<const llvm::Function *> Funcs,
               llvm::FunctionAnalysisManager &FAM,
               const LoopAnalysisReport *PreReport = nullptr);
  std::unique_ptr<AnalyzerReport> getReport() override;
  const LoopAnalysisReport &result() const;

private:
  llvm::FunctionAnalysisManager &FAM;
  LoopAnalysisReport Report;

  void analyzeProperty(llvm::Argument &A) override;
  void analyzeLoop(llvm::Argument &A);
  void handleStackFrame(StackFrame &Frame, std::stack<StackFrame> &DefUseChains,
                        std::set<llvm::Value *> &VisitedNodes);
  bool handleUser(StackFrame &Frame, llvm::Value &User,
                  std::stack<StackFrame> &DefUseChains,
                  std::set<llvm::Value *> &VisitedNodes);
  bool updateArgFlow(llvm::Argument &A);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
  void updateDefault(const llvm::Module &M);
  void updateDefault(const llvm::Function &F,
                     const std::map<unsigned, int> &Values);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_LOOPANALYZER_H
