#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/analysis/ParamNumberAnalyzer.h"
#include "ftg/analysis/TypeAnalyzer.h"
#include "ftg/constantanalysis/ConstAnalyzer.h"
#include "ftg/propanalysis/AllocSizeAnalyzer.h"
#include "ftg/propanalysis/ArrayAnalyzer.h"
#include "ftg/propanalysis/DirectionAnalyzer.h"
#include "ftg/propanalysis/FilePathAnalyzer.h"
#include "ftg/propanalysis/LoopAnalyzer.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/type/Type.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

#include <experimental/filesystem>
#include <fstream>

using namespace llvm;
using namespace clang::ast_matchers;
namespace fs = std::experimental::filesystem;

namespace ftg {

TargetLibAnalyzer::TargetLibAnalyzer(std::shared_ptr<SourceLoader> SL,
                                     std::shared_ptr<APILoader> AL,
                                     std::string ExternLibDir)
    : SL(SL), AL(AL) {
  loadExternals(ExternLibDir);
}

bool TargetLibAnalyzer::analyze() {
  if (!SL || !AL)
    return false;

  AC = AL->load();
  SC = SL->load();
  if (!SC)
    return false;

  auto &M = *const_cast<llvm::Module *>(&SC->getLLVMModule());
  std::vector<clang::ASTUnit *> ASTUnits;
  for (auto *ASTUnit : SC->getASTUnits()) {
    assert(ASTUnit && "Unexpected Program State");
    ASTUnits.push_back(ASTUnit);
  }

  AnalyzedTargetLib = std::make_unique<TargetLib>(AC);
  if (!AnalyzedTargetLib)
    return false;

  prepareLLVMAnalysis(M);

  auto Funcs = collectAnalyzableIRFunctions();
  Analyzers.emplace_back(
      std::make_unique<AllocSizeAnalyzer>(&Solver, Funcs, &AllocSizeReport));
  Analyzers.emplace_back(
      std::make_unique<ArrayAnalyzer>(&Solver, Funcs, FAM, &ArrayReport));
  Analyzers.emplace_back(std::make_unique<ConstAnalyzer>(ASTUnits));
  Analyzers.emplace_back(
      std::make_unique<DirectionAnalyzer>(&Solver, Funcs, &DirectionReport));
  Analyzers.emplace_back(
      std::make_unique<FilePathAnalyzer>(&Solver, Funcs, &FilePathReport));
  Analyzers.emplace_back(
      std::make_unique<LoopAnalyzer>(&Solver, Funcs, FAM, &LoopReport));
  Analyzers.emplace_back(
      std::make_unique<ParamNumberAnalyzer>(ASTUnits, M, AC));
  Analyzers.emplace_back(std::make_unique<TypeAnalyzer>(ASTUnits));

  for (auto &Analyzer : Analyzers) {
    if (!Analyzer)
      continue;

    Reports.emplace_back(Analyzer->getReport());
  }
  return true;
}

bool TargetLibAnalyzer::dump(std::string OutputFilePath) const {
  if (!AnalyzedTargetLib)
    return false;

  Json::Value Root = AnalyzedTargetLib->toJson();

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

const SourceCollection *TargetLibAnalyzer::getSourceCollection() const {
  return SC.get();
}

TargetLib &TargetLibAnalyzer::getTargetLib() { return *AnalyzedTargetLib; }

std::vector<const llvm::Function *>
TargetLibAnalyzer::collectAnalyzableIRFunctions() const {
  if (!SC)
    return {};

  std::vector<const llvm::Function *> Result;
  bool All = AC.size() == 0 ? true : false;
  for (const auto &F : SC->getLLVMModule()) {
    if (F.isDeclaration())
      continue;

    if (!All && AC.find(F.getName().str()) == AC.end())
      continue;

    Result.push_back(&F);
  }
  return Result;
}

void TargetLibAnalyzer::loadExternals(const std::string &ExternLibDir) {
  if (!fs::is_directory(ExternLibDir))
    return;

  std::unique_ptr<TargetLib> TL = std::make_unique<TargetLib>();
  for (auto &ExternFilePath : util::readDirectory(ExternLibDir)) {
    auto ExternJsonStr = util::readFile(ExternFilePath.c_str());
    if (ExternJsonStr.empty())
      continue;

    TL->fromJson(util::strToJson(ExternJsonStr));

    std::istringstream Iss(ExternJsonStr);
    Json::CharReaderBuilder Reader;
    Json::Value JsonValue;
    Json::parseFromStream(Reader, Iss, &JsonValue, nullptr);
    AllocSizeReport.fromJson(JsonValue);
    ArrayReport.fromJson(JsonValue);
    ConstReport.fromJson(JsonValue);
    DirectionReport.fromJson(JsonValue);
    FilePathReport.fromJson(JsonValue);
    LoopReport.fromJson(JsonValue);
    ParamNumberReport.fromJson(JsonValue);
    TargetReport.fromJson(JsonValue);
  }
  MergedExternLib = std::move(TL);
}

void TargetLibAnalyzer::prepareLLVMAnalysis(llvm::Module &M) {
  llvm::PassBuilder PB;
  llvm::LoopAnalysisManager LAM;
  llvm::CGSCCAnalysisManager CGAM;
  llvm::ModuleAnalysisManager MAM;
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

  llvm::LoopPassManager LPM;
  llvm::FunctionPassManager FPM;
  llvm::ModulePassManager MPM;
  FPM.addPass(PromotePass());
  FPM.addPass(SCCPPass());
  LPM.addPass(IndVarSimplifyPass());
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
  MPM.run(M, MAM);

  // For handle to indirect calls
  Solver.solve(M);
}

} // namespace ftg
