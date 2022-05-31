#include "ftg/utanalysis/UTAnalyzer.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/inputanalysis/DefAnalyzer.h"
#include "ftg/sourceanalysis/SourceAnalyzerImpl.h"
#include "ftg/tcanalysis/TCAnalyzerFactory.h"
#include <fstream>

using namespace ftg;

UTAnalyzer::UTAnalyzer(std::shared_ptr<SourceLoader> SL,
                       std::shared_ptr<APILoader> AL, std::string TargetPath,
                       std::string ExternPath, std::string UTType) {
  Loader = std::make_shared<UTLoader>(
      SL, AL, std::vector<std::string>({TargetPath, ExternPath}));
  initialize(UTType);
}

UTAnalyzer::UTAnalyzer(std::shared_ptr<UTLoader> Loader, std::string UTType)
    : Loader(Loader) {
  initialize(UTType);
}

bool UTAnalyzer::analyze() {
  if (!Extractor || !Loader)
    return false;
  Extractor->load();

  Analyzers.emplace_back(
      std::make_unique<SourceAnalyzerImpl>(Loader->getSourceCollection()));
  Analyzers.emplace_back(std::make_unique<DefAnalyzer>(
      Loader.get(), extractUnittests(), getLLVMFunctions(Loader->getAPIs()),
      std::make_shared<DebugInfoMap>(Loader->getSourceCollection())));

  for (auto &Analyzer : Analyzers) {
    if (!Analyzer)
      continue;
    Reports.emplace_back(Analyzer->getReport());
  }
  return true;
}

bool UTAnalyzer::dump(std::string OutputFilePath) {
  Json::Value Root;
  for (auto &Report : Reports) {
    if (!Report)
      continue;

    auto ReportJson = Report->toJson();
    for (auto MemberName : ReportJson.getMemberNames())
      Root[MemberName] = ReportJson[MemberName];
  }

  Json::StreamWriterBuilder Writer;
  std::ofstream Ofs(OutputFilePath);
  Ofs << Json::writeString(Writer, Root);
  Ofs.close();
  return true;
}

std::vector<Unittest> UTAnalyzer::extractUnittests() const {
  assert(Extractor && Loader && "Unexpected Program State");

  std::vector<Unittest> Result;
  for (auto &TCName : Extractor->getTCNames()) {
    const auto *UT = Extractor->getTC(TCName);
    if (!UT)
      continue;

    Result.push_back(*UT);
  }
  return Result;
}

std::set<llvm::Function *>
UTAnalyzer::getLLVMFunctions(const std::set<std::string> &FuncNames) const {
  if (!Loader)
    return {};

  std::set<llvm::Function *> Result;
  const auto &M = Loader->getSourceCollection().getLLVMModule();
  for (const auto &FuncName : FuncNames) {
    const auto *F = M.getFunction(FuncName);
    if (!F)
      continue;

    Result.insert(const_cast<llvm::Function *>(F));
  }
  return Result;
}

void UTAnalyzer::initialize(std::string UTType) {
  if (!Loader)
    return;

  auto Factory = createTCAnalyzerFactory(UTType);
  if (!Factory)
    return;

  Extractor = Factory->createTCExtractor(Loader->getSourceCollection());
}
