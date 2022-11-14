#ifndef FTG_PROPANALYSIS_LOOPANALYZER_H
#define FTG_PROPANALYSIS_LOOPANALYZER_H

#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/LoopAnalysisReport.h"
#include "ftg/propanalysis/StackFrame.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"

namespace ftg {

class LoopAnalyzer : public ArgFlowAnalyzer {
public:
  LoopAnalyzer(IndCallSolverMgr *Solver,
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
  bool isUsedAsInitializer(const llvm::ICmpInst &I, const llvm::Loop &L,
                           const std::set<llvm::Value *> &VisitedNodes);
  bool updateArgFlow(llvm::Argument &A);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_LOOPANALYZER_H
