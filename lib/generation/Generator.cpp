#include "ftg/generation/Generator.h"
#include "ftg/generation/CorpusGenerator.h"
#include "ftg/generation/FuzzerSrcGenerator.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace ftg {

void Generator::generate(std::string SrcDir,
                         std::set<std::string> PublicAPIList,
                         std::string TargetAnalyzerResultPath,
                         std::string UTAnalyzerResultPath,
                         std::string OutputDir) {
  if (!Loader.load(TargetAnalyzerResultPath, UTAnalyzerResultPath))
    return;

  // generate Fuzzers
  Fuzzers = initFuzzers();

  // generate source codes
  generateFuzzSource(Fuzzers, SrcDir, OutputDir);

  // prepare report
  exportReportJson(Fuzzers, PublicAPIList, SrcDir, OutputDir);

  // prepare corpus
  generateCorpus(Fuzzers, OutputDir);

  printNotGenerated();
}

std::vector<std::shared_ptr<Fuzzer>> Generator::initFuzzers() const {
  std::vector<std::shared_ptr<Fuzzer>> Fuzzers;
  for (const auto &Iter : Loader.getInputReport().getUnittests()) {
    Fuzzers.push_back(Fuzzer::create(Iter, Loader.getInputReport()));
  }
  assignUniqueName(Fuzzers);
  return Fuzzers;
}

void Generator::assignUniqueName(
    std::vector<std::shared_ptr<Fuzzer>> &Fuzzers) const {

  auto DuplicateGroups = getDuplicates(Fuzzers);
  if (DuplicateGroups.size() == 0)
    return;

  for (auto &DuplicateGroup : DuplicateGroups) {
    for (auto &F : DuplicateGroup) {
      assert(F && "Unexpected Program State");

      auto NewName = F->getUT().getFilePath();
      NewName = fs::path(NewName).filename();
      NewName = NewName + "_" + F->getName();
      F->setName(NewName);
    }
  }

  DuplicateGroups = getDuplicates(Fuzzers);

  // Need more precise approach to distinguish every test function by its name
  // if assert works.
  assert(DuplicateGroups.size() == 0 && "Unexpected Program State");
}

std::vector<std::vector<std::shared_ptr<Fuzzer>>>
Generator::getDuplicates(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers) const {

  std::map<std::string, std::vector<std::shared_ptr<Fuzzer>>> Table;

  for (auto &F : Fuzzers) {
    assert(F && "Unexpected Program State");

    auto Iter = Table.find(F->getName());
    if (Iter != Table.end()) {
      Iter->second.push_back(F);
      continue;
    }

    std::vector<std::shared_ptr<Fuzzer>> Fuzzers = {F};
    auto Result = Table.emplace(F->getName(), Fuzzers);
    assert(Result.second && "Unexpected Program State");
  }

  std::vector<std::vector<std::shared_ptr<Fuzzer>>> Result;
  for (auto Iter : Table) {
    if (Iter.second.size() < 2)
      continue;

    Result.push_back(Iter.second);
  }

  return Result;
}

void Generator::generateFuzzSource(
    std::vector<std::shared_ptr<Fuzzer>> &Fuzzers, std::string SrcDir,
    std::string OutputDir) const {
  FuzzerSrcGenerator Generator(Loader.getSourceReport());
  util::makeDir(OutputDir);

  unsigned GeneratedCnt = 0;
  for (auto &F : Fuzzers) {
    assert(F && "Unexpected Program State");
    if (!Generator.generate(*F, SrcDir, OutputDir))
      continue;
    GeneratedCnt++;
  }
  llvm::outs() << "[Fuzz Generator] Generated Fuzz Projects : " << GeneratedCnt
               << "\n";
}

void Generator::exportReportJson(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers,
                                 std::set<std::string> &PublicAPIList,
                                 std::string SrcPath, std::string OutputDir) {
  Reporter = std::make_unique<FuzzGenReporter>(PublicAPIList);
  for (auto &F : Fuzzers) {
    assert(F && "Unexpected Program State");
    Reporter->addFuzzer(*F);
  }

  // Add APIs that has no input parameter (forceful overwrite)
  const auto &ParamSizeMap = Loader.getParamNumberReport().getParamSizeMap();
  const auto &ParamBeginMap = Loader.getParamNumberReport().getParamBeginMap();
  const auto &DirectionReport = Loader.getDirectionReport();
  for (auto &PublicAPI : PublicAPIList) {
    auto SizeIter = ParamSizeMap.find(PublicAPI);
    if (SizeIter == ParamSizeMap.end()) {
      // TODO: All Public APIs should be analyzed in TargetAnalyzer
      llvm::outs() << "[W] Not Analyzed API: " << PublicAPI << "\n";
      continue;
    }
    auto ParamSize = SizeIter->second;
    auto BeginIter = ParamBeginMap.find(PublicAPI);
    auto BeginIndex = 0u;
    if (BeginIter != ParamBeginMap.end())
      BeginIndex = BeginIter->second;

    auto ActualParamSize = 0;
    if (ParamSize > BeginIndex)
      ActualParamSize = ParamSize - BeginIndex;

    if (ActualParamSize == 0) {
      Reporter->addNoInputAPI(PublicAPI);
      continue;
    }

    bool HasInputParam = false;
    for (unsigned S = BeginIndex; S < ParamSize; ++S) {
      if (!DirectionReport.has(PublicAPI, S) ||
          DirectionReport.get(PublicAPI, S) != Dir::Dir_Out) {
        HasInputParam = true;
        break;
      }
    }
    if (!HasInputParam)
      Reporter->addNoInputAPI(PublicAPI);
  }

  try {
    Reporter->saveReport(OutputDir);
  } catch (Json::Exception &E) {
    assert(false && "Unexpected Program State");
  }
}

void Generator::generateCorpus(std::vector<std::shared_ptr<Fuzzer>> &Fuzzers,
                               std::string OutputDir) const {

  OutputDir = OutputDir + PATH_SEPARATOR + "corpus";
  if (fs::exists(OutputDir))
    fs::remove_all(OutputDir);
  assert(fs::create_directories(OutputDir) && "Unexpected Program State");

  for (auto &F : Fuzzers) {
    assert(F && "Unexpected Program State");
    if (F->getFuzzInputMap().size() == 0)
      continue;

    generateCorpus(*F, OutputDir);
  }
}

void Generator::generateCorpus(const Fuzzer &F, std::string OutputDir) const {

  CorpusGenerator CorpusGen;
  auto Corpus = CorpusGen.generate(F);
  std::string OutputPath = OutputDir + PATH_SEPARATOR + F.getName() + ".corpus";
  assert(util::saveFile(OutputPath.c_str(), Corpus.c_str()));
}

const FuzzGenReporter &Generator::getFuzzGenReporter() const {
  assert(Reporter.get() && "FuzzGenReporter has not been initialized!");
  return *Reporter;
}

const Fuzzer *Generator::getFuzzer(std::string Name) const {

  auto Iter = std::find_if(Fuzzers.begin(), Fuzzers.end(),
                           [Name](const std::shared_ptr<Fuzzer> &F) {
                             assert(F && "Unexpected Program State");
                             return F->getName() == Name;
                           });

  if (Iter == Fuzzers.end())
    return nullptr;
  return Iter->get();
}

const GenLoader &Generator::getLoader() const { return Loader; }

void Generator::printNotGenerated() const {

  for (auto &F : Fuzzers) {
    if (F->getStatus() == FUZZABLE_SRC_GENERATED)
      continue;
    llvm::outs() << "[W] Not Generated Fuzzer [" << F->getName() << "]\n";

    auto &FINM = F->getFuzzNoInputMap();
    for (auto &Iter : FINM) {

      llvm::outs() << "[W] No Input: " << *Iter.second << "\n";
    }
  }
}

} // namespace ftg
