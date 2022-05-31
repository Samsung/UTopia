//===-- FuzzerSrcGenerator.cpp - Implementation of FuzzerSrcGenerator -----===//

#include "ftg/generation/FuzzerSrcGenerator.h"
#include "ftg/generation/InputMutator.h"
#include "ftg/generation/ProtobufMutator.h"
#include "ftg/generation/UTModify.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace ftg;

FuzzerSrcGenerator::FuzzerSrcGenerator(const SourceAnalysisReport &SourceReport)
    : SourceReport(SourceReport) {
  clang::IntrusiveRefCntPtr<llvm::vfs::OverlayFileSystem> OverlayFS =
      new llvm::vfs::OverlayFileSystem(llvm::vfs::getRealFileSystem());
  clang::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID =
      new clang::DiagnosticIDs();
  DiagOpts = new clang::DiagnosticOptions();
  FManager = new clang::FileManager(clang::FileSystemOptions(), OverlayFS);
  DiagnosticPrinter = std::make_unique<clang::TextDiagnosticPrinter>(
      llvm::outs(), DiagOpts.get());
  Diagnostics = std::make_unique<clang::DiagnosticsEngine>(
      DiagID, DiagOpts.get(), DiagnosticPrinter.get(), false);
  SManager = std::make_unique<clang::SourceManager>(*Diagnostics, *FManager);
  LangOpts = std::make_unique<clang::LangOptions>();
}

bool FuzzerSrcGenerator::generate(Fuzzer &F, const std::string &SrcDir,
                                  const std::string &OutDir) {
  if (F.getStatus() != FUZZABLE)
    return false;

  auto FuzzerSrcPath = fs::path(OutDir) / F.getName();
  util::copy(SrcDir, FuzzerSrcPath, true);

  auto ProjectDir = SourceReport.getSrcBaseDir();
  auto UTSrcDirRelPath = util::getParentPath(
      util::getRelativePath(F.getUT().getFilePath(), ProjectDir));

  // Creation should be fixed if other mutator added
  std::unique_ptr<InputMutator> Mutator = std::make_unique<ProtobufMutator>();
  Mutator->initFromFuzzer(F, SourceReport);
  Mutator->genEntry(FuzzerSrcPath / UTSrcDirRelPath);

  UTModify Modify(F, SourceReport);
  for (auto Iter : Modify.getReplacements()) {
    auto FilePath = Iter.first;
    assert(!FilePath.empty() && "Unexpected Program State");
    auto Replaces = Iter.second;

    auto OutPath = util::rebasePath(FilePath, ProjectDir, FuzzerSrcPath);
    applyReplacements(FilePath, OutPath, Replaces);
  }

  for (auto Iter : Modify.getNewFiles()) {
    auto FilePath = Iter.first;
    assert(!FilePath.empty() && "Unexpected Program State");
    auto Content = Iter.second;

    auto OutPath = util::rebasePath(FilePath, ProjectDir, FuzzerSrcPath);
    util::saveFile(OutPath.c_str(), Content.c_str());
  }

  // update FTG build paths
  F.setRelativeUTDir(UTSrcDirRelPath);

  F.setStatus(FUZZABLE_SRC_GENERATED);
  return true;
}

void FuzzerSrcGenerator::applyReplacements(
    const std::string &OrgFilePath, const std::string &OutFilePath,
    const clang::tooling::Replacements &Replaces) {
  auto ID = SManager->getOrCreateFileID(FManager->getFile(OrgFilePath).get(),
                                        clang::SrcMgr::C_User);
  clang::Rewriter Rewrite(*SManager, *LangOpts);
  clang::tooling::applyAllReplacements(Replaces, Rewrite);
  std::error_code EC;
  llvm::raw_fd_ostream OutFile(OutFilePath, EC, llvm::sys::fs::F_None);
  Rewrite.getEditBuffer(ID).write(OutFile);
}
