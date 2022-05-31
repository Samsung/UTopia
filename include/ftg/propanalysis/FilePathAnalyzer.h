#ifndef FTG_PROPANALYSIS_FILEPATHANALYZER_H
#define FTG_PROPANALYSIS_FILEPATHANALYZER_H

#include "ftg/propanalysis/ArgFlowAnalyzer.h"
#include "ftg/propanalysis/FilePathAnalysisReport.h"
#include "ftg/propanalysis/StackFrame.h"
#include "llvm/IR/Argument.h"

namespace ftg {

class FilePathAnalyzer : public ArgFlowAnalyzer {

public:
  FilePathAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                   std::vector<const llvm::Function *> Funcs,
                   const FilePathAnalysisReport *PreReport = nullptr);
  std::unique_ptr<AnalyzerReport> getReport() override;
  const FilePathAnalysisReport &report() const;

protected:
  void analyzeProperty(llvm::Argument &A) override;

private:
  FilePathAnalysisReport Report;

  void analyzeFilePath(llvm::Argument &A);
  void handleStackFrame(StackFrame &Frame, std::stack<StackFrame> &DefUseChains,
                        std::set<llvm::Value *> &VisitedNodes);
  void handleUser(StackFrame &Frame, llvm::Value &User,
                  std::stack<StackFrame> &DefUseChains,
                  std::set<llvm::Value *> &VisitedNodes);
  bool updateArgFlow(llvm::Argument &A);
  void updateDefault(const llvm::Module &M);
  void updateDefault(const llvm::Function &F, unsigned ArgIdx);
  void updateFieldFlow(ArgFlow &AF, std::vector<int> Indices = {});
};

} // namespace ftg

#endif // FTG_PROPANALYSIS_FILEPATHANALYZER_H
