#include "ftg/targetanalysis/TargetLibAnalyzer.h"
#include "ftg/analysis/ParamNumberAnalyzer.h"
#include "ftg/constantanalysis/ConstAnalyzer.h"
#include "ftg/indcallsolver/IndCallSolverImpl.h"
#include "ftg/propanalysis/AllocAnalyzer.h"
#include "ftg/propanalysis/ArrayAnalyzer.h"
#include "ftg/propanalysis/DirectionAnalyzer.h"
#include "ftg/propanalysis/FilePathAnalyzer.h"
#include "ftg/propanalysis/LoopAnalyzer.h"
#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/targetanalysis/TargetLibExportUtil.h"
#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/type/Type.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/FileUtil.h"
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

  AnalyzedTargetLib = std::make_unique<TargetLib>(M, AC);
  if (!AnalyzedTargetLib)
    return false;

  parseTypes(ASTUnits);
  prepareLLVMAnalysis(M);

  auto Funcs = collectAnalyzableIRFunctions();
  Analyzers.emplace_back(
      std::make_unique<AllocAnalyzer>(Solver, Funcs, &AllocReport));
  Analyzers.emplace_back(
      std::make_unique<ArrayAnalyzer>(Solver, Funcs, FAM, &ArrayReport));
  Analyzers.emplace_back(std::make_unique<ConstAnalyzer>(ASTUnits));
  Analyzers.emplace_back(
      std::make_unique<DirectionAnalyzer>(Solver, Funcs, &DirectionReport));
  Analyzers.emplace_back(
      std::make_unique<FilePathAnalyzer>(Solver, Funcs, &FilePathReport));
  Analyzers.emplace_back(
      std::make_unique<LoopAnalyzer>(Solver, Funcs, FAM, &LoopReport));
  Analyzers.emplace_back(
      std::make_unique<ParamNumberAnalyzer>(ASTUnits, M, AC));

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

  Json::CharReaderBuilder Reader;
  Json::Value Root;
  std::istringstream StrStream(toJsonString(*AnalyzedTargetLib));
  if (!Json::parseFromStream(Reader, StrStream, &Root, nullptr))
    return false;

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

    if (!All && AC.find(F.getName()) == AC.end())
      continue;

    Result.push_back(&F);
  }
  return Result;
}

void TargetLibAnalyzer::loadExternals(const std::string &ExternLibDir) {
  if (!fs::is_directory(ExternLibDir))
    return;

  TargetLibLoader Loader;
  for (auto &ExternFilePath : util::readDirectory(ExternLibDir)) {
    auto ExternJsonStr = util::readFile(ExternFilePath.c_str());
    if (ExternJsonStr.empty())
      continue;

    Loader.load(ExternJsonStr);

    std::istringstream Iss(ExternJsonStr);
    Json::CharReaderBuilder Reader;
    Json::Value JsonValue;
    Json::parseFromStream(Reader, Iss, &JsonValue, nullptr);
    AllocReport.fromJson(JsonValue);
    ArrayReport.fromJson(JsonValue);
    ConstReport.fromJson(JsonValue);
    DirectionReport.fromJson(JsonValue);
    FilePathReport.fromJson(JsonValue);
    LoopReport.fromJson(JsonValue);
    ParamNumberReport.fromJson(JsonValue);
  }
  MergedExternLib = Loader.takeReport();
}

void TargetLibAnalyzer::parseTypes(std::vector<clang::ASTUnit *> &ASTUnits) {
  const std::string Tag = "Tag";
  for (auto *ASTUnit : ASTUnits) {
    if (!ASTUnit)
      continue;

    for (const auto &Node :
         match(enumDecl().bind(Tag), ASTUnit->getASTContext())) {
      auto *Record = Node.getNodeAs<clang::EnumDecl>(Tag);
      if (!Record)
        continue;

      AnalyzedTargetLib->addEnum(
          createEnum(*const_cast<clang::EnumDecl *>(Record)));
    }

    for (const auto &Node :
         match(recordDecl().bind(Tag), ASTUnit->getASTContext())) {
      const auto *Record = Node.getNodeAs<clang::RecordDecl>(Tag);
      if (!Record)
        continue;

      auto Name = util::getTypeName(*Record);
      if (Name.empty())
        continue;

      if (!Record->isCompleteDefinition() || !util::isCStyleStruct(*Record) ||
          AnalyzedTargetLib->getStruct(Name))
        continue;

      AnalyzedTargetLib->addStruct(
          createStruct(*const_cast<clang::RecordDecl *>(Record)));
    }

    for (const auto &Node :
         match(typedefType().bind(Tag), ASTUnit->getASTContext())) {
      auto *Record = Node.getNodeAs<clang::TypedefType>(Tag);
      if (!Record)
        continue;

      auto *TD = Record->getAsTagDecl();
      if (!TD)
        continue;

      auto Name = util::getTypeName(*TD);
      if (Name.empty())
        continue;

      auto *D = Record->getDecl();
      if (!D)
        continue;

      auto TDName = D->getNameAsString();
      if (TDName != Name)
        AnalyzedTargetLib->addTypedef(std::make_pair(TDName, Name));
    }
  }
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
  Solver = std::make_shared<IndCallSolverImpl>();
  assert(Solver && "Unexpected Program State");
  Solver->solve(M);
}

std::shared_ptr<Struct>
TargetLibAnalyzer::createStruct(clang::RecordDecl &clangS) {
  std::shared_ptr<Struct> ret = std::make_shared<Struct>();
  ret->setName(util::getTypeName(clangS));
  for (clang::FieldDecl *clangF : clangS.fields()) {
    ret->addField(createField(*clangF, &*ret));
  }
  return ret;
}

std::shared_ptr<Enum> TargetLibAnalyzer::createEnum(clang::EnumDecl &clangE) {
  std::shared_ptr<Enum> ret = std::make_shared<Enum>();
  ret->setName(clangE.getQualifiedNameAsString());
  ret->setScoped(clangE.isScoped());
  ret->setScopedUsingClassTag(clangE.isScopedUsingClassTag());
  for (clang::EnumConstantDecl *field : clangE.enumerators()) {
    ret->addElement(std::make_shared<EnumConst>(
        field->getNameAsString(), field->getInitVal().getExtValue(), &*ret));
  }
  return ret;
}

std::shared_ptr<Field> TargetLibAnalyzer::createField(clang::FieldDecl &FD,
                                                      Struct *S) {
  std::shared_ptr<Field> Result = std::make_shared<Field>(S);
  Result->setVarName(FD.getNameAsString());
  Result->setType(Type::createType(FD.getType(), FD.getASTContext(),
                                   Result.get(), nullptr, nullptr,
                                   AnalyzedTargetLib.get()));
  Result->setIndex(FD.getFieldIndex());
  return Result;
}

} // namespace ftg
