//===-- InputMutator.cpp - Implementation of InputMutator -----------------===//

#include "ftg/generation/InputMutator.h"
#include "ftg/generation/UTModify.h"

using namespace ftg;

const std::string InputMutator::DefaultEntryName = "fuzz_entry.cc";

void InputMutator::initFromFuzzer(const Fuzzer &F,
                                  const SourceAnalysisReport &SourceReport) {
  for (auto IDInput : F.getFuzzInputMap()) {
    auto FuzzingInput = IDInput.second;
    addInput(*FuzzingInput);
  }
  auto DependentHeaders =
      SourceReport.getIncludedHeaders(F.getUT().getFilePath());
  Headers.insert(DependentHeaders.begin(), DependentHeaders.end());
}
