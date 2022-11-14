#ifndef FTG_TARGETANALYSIS_TARGETLIBANALYZER_H
#define FTG_TARGETANALYSIS_TARGETLIBANALYZER_H

#include "ftg/Analyzer.h"
#include "ftg/analysis/ParamNumberAnalysisReport.h"
#include "ftg/analysis/TypeAnalysisReport.h"
#include "ftg/apiloader/APILoader.h"
#include "ftg/constantanalysis/ConstAnalyzerReport.h"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/propanalysis/AllocSizeAnalysisReport.h"
#include "ftg/propanalysis/ArrayAnalysisReport.h"
#include "ftg/propanalysis/DirectionAnalysisReport.h"
#include "ftg/propanalysis/FilePathAnalysisReport.h"
#include "ftg/propanalysis/LoopAnalysisReport.h"
#include "ftg/sourceloader/SourceLoader.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/GlobalDef.h"
#include "llvm/Passes/PassBuilder.h"

namespace ftg {

class TargetLibAnalyzer {
public:
  TargetLibAnalyzer(std::shared_ptr<SourceLoader> SL,
                    std::shared_ptr<APILoader> AL,
                    std::string ExternLibDir = "");
  bool analyze();
  bool dump(std::string OutputFilePath) const;
  const SourceCollection *getSourceCollection() const;
  TargetLib &getTargetLib();

private:
  std::shared_ptr<SourceLoader> SL;
  std::shared_ptr<APILoader> AL;
  std::unique_ptr<TargetLib> MergedExternLib;
  std::unique_ptr<SourceCollection> SC;
  std::set<std::string> AC;
  std::unique_ptr<TargetLib> AnalyzedTargetLib;
  IndCallSolverMgr Solver;
  llvm::FunctionAnalysisManager FAM;
  std::vector<std::unique_ptr<Analyzer>> Analyzers;
  std::vector<std::unique_ptr<AnalyzerReport>> Reports;
  AllocSizeAnalysisReport AllocSizeReport;
  ArrayAnalysisReport ArrayReport;
  ConstAnalyzerReport ConstReport;
  DirectionAnalysisReport DirectionReport;
  FilePathAnalysisReport FilePathReport;
  LoopAnalysisReport LoopReport;
  ParamNumberAnalysisReport ParamNumberReport;
  TypeAnalysisReport TargetReport;

  std::vector<const llvm::Function *> collectAnalyzableIRFunctions() const;
  void loadExternals(const std::string &ExternLibDir);
  void prepareLLVMAnalysis(llvm::Module &M);
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIBANALYZER_H
