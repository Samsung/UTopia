//===-- InputMutator.h - Abstract class for fuzz input mutator -*- C++ -*- ===//
///
/// \file
/// InputMutator is abstract class for various fuzz input mutator.
/// Each mutator should implement generation logic for entry file.
/// Entry file should contain main entry function of the fuzzer and it should
/// fetch mutation result from the mutator.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_INPUTMUTATOR_H
#define FTG_GENERATION_INPUTMUTATOR_H

#include "ftg/generation/FuzzInput.h"
#include "ftg/generation/Fuzzer.h"
#include "ftg/generation/UTModify.h"
#include "ftg/sourceanalysis/SourceAnalysisReport.h"

#include <set>
#include <string>

namespace ftg {
class InputMutator {
public:
  static const std::string DefaultEntryName;
  virtual ~InputMutator() = default;

  /// Generates entry file of the fuzzer.
  virtual void genEntry(const std::string &OutDir,
                        const std::string &FileName = DefaultEntryName) = 0;

  /// Adds fuzzing input to mutation target.
  virtual void addInput(FuzzInput &Input) = 0;

  /// Adds whole fuzzing input of the fuzzer as mutation target.
  void initFromFuzzer(const Fuzzer &F,
                      const SourceAnalysisReport &SourceReport);

protected:
  std::set<std::string> Headers = {'"' + UTModify::HeaderName + '"'};
};
} // namespace ftg

#endif // FTG_GENERATION_INPUTMUTATOR_H
