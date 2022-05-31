#ifndef FTG_UTANALYSIS_UTANALYZER_H
#define FTG_UTANALYSIS_UTANALYZER_H

#include "ftg/Analyzer.h"
#include "ftg/tcanalysis/TCExtractor.h"
#include "ftg/utanalysis/UTLoader.h"

namespace ftg {

class UTAnalyzer {

public:
  UTAnalyzer(std::shared_ptr<SourceLoader> SL, std::shared_ptr<APILoader> AL,
             std::string TargetPath, std::string ExternPath,
             std::string UTType);
  UTAnalyzer(std::shared_ptr<UTLoader> Loader, std::string UTType);
  bool analyze();
  bool dump(std::string OutputFilePath);

private:
  std::shared_ptr<UTLoader> Loader;
  std::unique_ptr<TCExtractor> Extractor;
  std::vector<std::unique_ptr<Analyzer>> Analyzers;
  std::vector<std::unique_ptr<AnalyzerReport>> Reports;

  std::vector<Unittest> extractUnittests() const;
  std::set<llvm::Function *>
  getLLVMFunctions(const std::set<std::string> &FuncNames) const;
  void initialize(std::string UTType);
};

} // namespace ftg

#endif // FTG_UTANALYSIS_UTANALYZER_H
