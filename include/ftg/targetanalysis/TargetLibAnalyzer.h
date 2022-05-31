#ifndef FTG_TARGETANALYSIS_TARGETLIBANALYZER_H
#define FTG_TARGETANALYSIS_TARGETLIBANALYZER_H

#include "ftg/Analyzer.h"
#include "ftg/analysis/ParamNumberAnalysisReport.h"
#include "ftg/apiloader/APILoader.h"
#include "ftg/constantanalysis/ConstAnalyzerReport.h"
#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/propanalysis/AllocAnalysisReport.h"
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
  std::shared_ptr<IndCallSolver> Solver;
  llvm::FunctionAnalysisManager FAM;
  std::vector<std::unique_ptr<Analyzer>> Analyzers;
  std::vector<std::unique_ptr<AnalyzerReport>> Reports;
  AllocAnalysisReport AllocReport;
  ArrayAnalysisReport ArrayReport;
  ConstAnalyzerReport ConstReport;
  DirectionAnalysisReport DirectionReport;
  FilePathAnalysisReport FilePathReport;
  LoopAnalysisReport LoopReport;
  ParamNumberAnalysisReport ParamNumberReport;

  std::shared_ptr<Enum> createEnum(clang::EnumDecl &ED);
  std::shared_ptr<Field> createField(clang::FieldDecl &FD, Struct *S);
  std::shared_ptr<Struct> createStruct(clang::RecordDecl &RD);
  std::vector<const llvm::Function *> collectAnalyzableIRFunctions() const;
  void loadExternals(const std::string &ExternLibDir);
  void parseTypes(std::vector<clang::ASTUnit *> &ASTUnits);
  void prepareLLVMAnalysis(llvm::Module &M);
};

} // namespace ftg

#endif // FTG_TARGETANALYSIS_TARGETLIBANALYZER_H
