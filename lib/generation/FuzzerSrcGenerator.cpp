//===-- FuzzerSrcGenerator.cpp - Implementation of FuzzerSrcGenerator -----===//

#include "ftg/generation/FuzzerSrcGenerator.h"
#include "ftg/generation/InputMutator.h"
#include "ftg/generation/ProtobufMutator.h"
#include "ftg/generation/SrcGenerator.h"
#include "ftg/generation/UTModify.h"
#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace ftg;

FuzzerSrcGenerator::FuzzerSrcGenerator(const SourceAnalysisReport &SourceReport)
    : SourceReport(SourceReport) {}

bool FuzzerSrcGenerator::generate(Fuzzer &F, const std::string &SrcDir,
                                  const std::string &OutDir) {
  if (F.getStatus() != FUZZABLE)
    return false;

  auto FuzzerSrcPath = fs::path(OutDir) / F.getName();
  util::copy(SrcDir, FuzzerSrcPath, true);

  auto ProjectDir = SourceReport.getSrcBaseDir();
  auto UTSrcDirRelPath = util::getParentPath(
      util::getRelativePath(F.getUT().getFilePath(), ProjectDir));
  auto Signature = SrcGenerator::genSignature(F.getUT().getName());

  // Creation should be fixed if other mutator added
  std::unique_ptr<InputMutator> Mutator = std::make_unique<ProtobufMutator>();
  Mutator->initFromFuzzer(F, SourceReport);
  Mutator->genEntry(FuzzerSrcPath / UTSrcDirRelPath,
                    InputMutator::DefaultEntryName, Signature);

  UTModify Modify(F, SourceReport);
  Modify.genModifiedSrcs(ProjectDir, FuzzerSrcPath, Signature);

  // update FTG build paths
  F.setRelativeUTDir(UTSrcDirRelPath);

  F.setStatus(FUZZABLE_SRC_GENERATED);
  return true;
}
