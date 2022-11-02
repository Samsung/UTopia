#include "ftg/generation/Fuzzer.h"

namespace ftg {

std::shared_ptr<Fuzzer> Fuzzer::create(const Unittest &UT,
                                       const InputAnalysisReport &InputReport) {
  auto Result = std::make_shared<Fuzzer>(UT);
  if (!Result)
    return Result;

  Result->prepareFuzzInputs(InputReport);
  return Result;
}

Fuzzer::Fuzzer(const Unittest &UT) : Name(UT.getID()), UT(&UT) {}

void Fuzzer::prepareFuzzInputs(const InputAnalysisReport &InputReport) {

  assert(UT && "Unexpected Program State");

  auto &APICalls = UT->getAPICalls();
  if (APICalls.size() == 0) {
    ProjectStatus = NOT_FUZZABLE_NO_INPUT;
    return;
  }

  // Get Def IDs that are used in a UT.
  std::set<unsigned> DefIDs;
  for (auto &APICall : APICalls) {
    for (auto &APIArg : APICall.Args) {
      DefIDs.insert(APIArg.DefIDs.begin(), APIArg.DefIDs.end());
    }
  }

  // Get Definitions
  std::vector<std::shared_ptr<const Definition>> DefPtrs;
  const auto &DefMap = InputReport.getDefMap();
  for (auto DefID : DefIDs) {
    auto Iter = DefMap.find(DefID);
    assert(Iter != DefMap.end() && "Unexpected Program State");
    DefPtrs.push_back(Iter->second);
  }
  FuzzInputGenerator InputGenerator;
  std::tie(DefIDFuzzInputMap, DefIDFuzzNoInputMap) =
      InputGenerator.generate(DefPtrs);
  if (!DefIDFuzzInputMap.empty())
    ProjectStatus = FUZZABLE;
}

const Unittest &Fuzzer::getUT() const {

  assert(UT && "Unexpected Program State");
  return *UT;
}

const std::map<unsigned, std::shared_ptr<FuzzInput>> &
Fuzzer::getFuzzInputMap() const {
  return DefIDFuzzInputMap;
}
const std::map<unsigned, std::shared_ptr<FuzzNoInput>> &
Fuzzer::getFuzzNoInputMap() const {
  return DefIDFuzzNoInputMap;
}

std::string Fuzzer::getName() const { return Name; }

void Fuzzer::setName(std::string Name) { this->Name = Name; }

void Fuzzer::addReplace(const clang::tooling::Replacement &Replace) {
  llvm::Error Err = (SrcReplacements[Replace.getFilePath().str()].add(Replace));
  if (Err) {
    llvm::errs() << "[WARN] overlap on making Replace\n";
  }
  llvm::outs() << "(addReplace) " << Replace.toString() << "\n";
}

std::map<std::string, clang::tooling::Replacements> &
Fuzzer::getSrcReplacements() {
  return SrcReplacements;
}

void Fuzzer::setUTBottomString(std::string FilePath,
                               std::string UTBottomString) {
  UTBottomStringMap[FilePath] = UTBottomString;
}

std::string Fuzzer::getUTBottomString(std::string FilePath) {
  return (UTBottomStringMap.find(FilePath) != UTBottomStringMap.end())
             ? UTBottomStringMap[FilePath]
             : "";
}

void Fuzzer::setStatus(FuzzStatus Status) { ProjectStatus = Status; }

FuzzStatus Fuzzer::getStatus() const { return ProjectStatus; }

void Fuzzer::setRelativeUTDir(std::string Path) { this->RelativeUTDir = Path; }

std::string Fuzzer::getRelativeUTDir() const { return RelativeUTDir; }

bool Fuzzer::isFuzzableInput(unsigned int DefID) const {
  return DefIDFuzzInputMap.find(DefID) != DefIDFuzzInputMap.end();
}

} // namespace ftg
