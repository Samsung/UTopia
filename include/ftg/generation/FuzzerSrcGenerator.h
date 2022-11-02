//===-- FuzzerSrcGenerator.h - Code generator of fuzzer ---------*- C++ -*-===//
///
/// \file
/// Defines code generator of fuzzer.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_FUZZERSRCGENERATOR_H
#define FTG_GENERATION_FUZZERSRCGENERATOR_H

#include "ftg/generation/Fuzzer.h"
#include "ftg/sourceanalysis/SourceAnalysisReport.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include <string>

namespace ftg {

/// Code generator of fuzzer. It orchestrates sub-generators to generate
/// complete fuzzer codes.
class FuzzerSrcGenerator {
public:
  /// \param SourceReport Expected to be needed for getting header lists of UT
  FuzzerSrcGenerator(const SourceAnalysisReport &SourceReport);

  /// Generate complete source codes of fuzzer.
  /// \param F Target fuzzer to generate.
  /// \param SrcDir Path of unit test sources which will be base of the fuzzer.
  /// \param OutDir Output directory path. Codes will be generated under
  ///               \c OutDir/FuzzerName.
  /// \return true if generation succeed else false.
  bool generate(Fuzzer &F, const std::string &SrcDir,
                const std::string &OutDir);

private:
  const SourceAnalysisReport &SourceReport;
};
} // namespace ftg

#endif // FTG_GENERATION_FUZZERSRCGENERATOR_H
