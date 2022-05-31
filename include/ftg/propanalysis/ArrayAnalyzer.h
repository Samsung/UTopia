#ifndef PROPANALYSIS_ARRAYANALYZER_H
#define PROPANALYSIS_ARRAYANALYZER_H

#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/ArrayAnalysisReport.h"
#include "ftg/propanalysis/StackFrame.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"

namespace ftg {

class ArrayAnalyzer : public ArgFlowAnalyzer {

public:
  ArrayAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                std::vector<const llvm::Function *> Funcs,
                llvm::FunctionAnalysisManager &FAM,
                const ArrayAnalysisReport *PreReport = nullptr);

  std::unique_ptr<AnalyzerReport> getReport() override;
  const ArrayAnalysisReport &result() const;

private:
  llvm::FunctionAnalysisManager &FAM;
  std::unique_ptr<ArrayAnalysisReport> Report;

  void analyzeProperty(llvm::Argument &A) override;
  void analyzeArray(llvm::Argument &A);
  void analyzeArrayLen(llvm::Instruction &I, ArgFlow &AF);
  llvm::Value *getTrackingVariable(llvm::Loop &L);
  llvm::Value *getRelatedIndV(llvm::Loop &L, llvm::ScalarEvolution &SEA);
  void handleStackFrame(StackFrame &Frame, std::stack<StackFrame> &DefUseChains,
                        std::set<llvm::Value *> &VisitedNodes);
  void handleUser(StackFrame &Frame, llvm::Value &User,
                  std::stack<StackFrame> &DefUseChains,
                  std::set<llvm::Value *> &VisitedNodes);
  bool updateArgFlow(llvm::Argument &A);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
  void updateDefault(const llvm::Module &M);
  void updateDefault(const llvm::Function &F,
                     const std::map<unsigned, int> &Values);
};

} // namespace ftg

#endif
