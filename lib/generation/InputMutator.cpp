//===-- InputMutator.cpp - Implementation of InputMutator -----------------===//

#include "ftg/generation/InputMutator.h"
#include "ftg/generation/UTModify.h"

using namespace ftg;

const std::string InputMutator::DefaultEntryName = "fuzz_entry.cc";

void InputMutator::initFromFuzzer(const Fuzzer &F,
                                  const SourceAnalysisReport &SourceReport) {
  for (auto IDInput : F.getFuzzInputMap()) {
    auto FuzzingInput = IDInput.second;
    assert(FuzzingInput && "Unexpected Program State");
    if (FuzzingInput->getCopyFrom())
      continue;
    addInput(*FuzzingInput);
  }
  auto UTHeaders = SourceReport.getIncludedHeaders(F.getUT().getFilePath());
  DependentHeaders.insert(DependentHeaders.end(), UTHeaders.begin(),
                          UTHeaders.end());
}

std::vector<std::string> InputMutator::getHeaders() const {
  auto Result = DependentHeaders;
  Result.insert(Result.end(), AdditionalHeaders.begin(),
                AdditionalHeaders.end());
  // autofuzz header should be included in last to minimize side effects
  Result.emplace_back('"' + UTModify::HeaderName + '"');
  return Result;
}

void InputMutator::addHeader(const std::string &Header) {
  AdditionalHeaders.emplace(Header);
}
