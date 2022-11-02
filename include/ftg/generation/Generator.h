#ifndef FTG_GENERATION_GENERATOR_H
#define FTG_GENERATION_GENERATOR_H

#include "ftg/generation/FuzzGenReporter.h"
#include "ftg/generation/Fuzzer.h"
#include "ftg/generation/GenLoader.h"
#include "ftg/targetanalysis/TargetLib.h"

namespace ftg {

// Class for generating fuzz source.
class Generator {

public:
  Generator() = default;
  ~Generator() = default;

  void generate(std::string SrcDir, std::set<std::string> PublicAPIList,
                std::string TargetAnalyzerResultPath,
                std::string UTAnalyzerResultPath, std::string OutputDir);

  const FuzzGenReporter &getFuzzGenReporter() const;
  const Fuzzer *getFuzzer(std::string Name) const;
  const GenLoader &getLoader() const;

private:
  GenLoader Loader;
  std::vector<std::shared_ptr<Fuzzer>> Fuzzers;
  std::unique_ptr<FuzzGenReporter> Reporter;

  std::vector<std::shared_ptr<Fuzzer>> initFuzzers() const;
  void assignUniqueName(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers) const;
  std::vector<std::vector<std::shared_ptr<Fuzzer>>>
  getDuplicates(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers) const;

  // Function for generating fuzz source.
  void generateFuzzSource(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers,
                          std::string SrcDir, std::string OutputDir) const;

  // generate report json file
  void exportReportJson(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers,
                        std::set<std::string> &PublicAPIList,
                        std::string SrcPath, std::string OutputDir);

  void generateCorpus(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers,
                      std::string OutputDir) const;
  void generateCorpus(const Fuzzer &F, std::string OutputDir) const;

  void printNotGenerated() const;
};

} // end namespace ftg

#endif // FTG_GENERATION_GENERATOR_H
