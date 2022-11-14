#ifndef FTG_PROPANALYSIS_DIRECTIONANALYZER_H
#define FTG_PROPANALYSIS_DIRECTIONANALYZER_H

#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include "ftg/propanalysis/StackFrame.h"

namespace ftg {

class DirectionAnalyzer : public ArgFlowAnalyzer {

public:
  DirectionAnalyzer(IndCallSolverMgr *Solver,
                    const std::vector<const llvm::Function *> &Funcs,
                    const DirectionAnalysisReport *PreReport = nullptr);

  std::unique_ptr<AnalyzerReport> getReport() override;
  const DirectionAnalysisReport &result() const;

private:
  std::unique_ptr<DirectionAnalysisReport> Report;

  bool isPtrToVal(const llvm::Value &V) const;
  void analyzeProperty(llvm::Argument &A) override;
  void analyzeDirection(llvm::Argument &A);
  void handleStackFrame(StackFrame &Frame, std::stack<StackFrame> &DefUseChains,
                        std::set<llvm::Value *> &VisitedNodes);
  void handleUser(StackFrame &Frame, llvm::Value &User,
                  std::stack<StackFrame> &DefUseChains,
                  std::set<llvm::Value *> &VisitedNodes);
  bool updateArgFlow(llvm::Argument &A);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
  void updateDefault(const llvm::Module &M);
  void updateDefault(const llvm::Function &F,
                     const std::map<unsigned, ArgDir> &Values);
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_DIRECTIONANALYZER_H
