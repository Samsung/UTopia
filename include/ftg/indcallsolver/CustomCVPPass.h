//===- CustomCVP.h - Propagate called values -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements Custom Called Value Propagation.
// This propagation is almost the same as LLVM Called Value Propagation,
// but with new features and some changes.
//
// LLVM Called Value Propagation implements a transformation
// that attaches !callees metadata to indirect call sites.
// For a given call site, the metadata, if present,
// indicates the set of functions the call site could possibly target at
// run-time. This metadata is added to indirect call sites when the set of
// possible targets can be determined by analysis and is known to be small. The
// analysis driving the transformation is similar to constant propagation and
// makes uses of the generic sparse propagation solver.
//
//===----------------------------------------------------------------------===//

#ifndef FTG_INDCALLSOLVER_CUSTOMCVPPASS_H
#define FTG_INDCALLSOLVER_CUSTOMCVPPASS_H

#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"

namespace ftg {

class CustomCVPPass : public llvm::PassInfoMixin<CustomCVPPass> {

public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
};

} // namespace ftg

#endif // FTG_INDCALLSOLVER_CUSTOMCVPPASS_H
