#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/indcallsolver/CustomCVPPass.h"
#include "ftg/indcallsolver/GlobalInitializerSolver.h"
#include "ftg/indcallsolver/LLVMWalker.h"
#include "ftg/indcallsolver/TBAASimpleSolver.h"
#include "ftg/indcallsolver/TBAAVirtSolver.h"
#include "ftg/indcallsolver/TestTypeVirtSolver.h"
#include "llvm/Analysis/TypeMetadataUtils.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Passes/PassBuilder.h"

using namespace ftg;
using namespace llvm;

void IndCallSolverMgr::solve(Module &M) {
  ModulePassManager MPM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
  PB.registerModuleAnalyses(MAM);
  // add customized CVP pass
  MPM.addPass(CustomCVPPass());
  MPM.run(M, MAM);

  LLVMWalker Walker;
  GlobalInitializerSolverHandler GISHandler;
  TBAASimpleSolverHandler TSSHandler;
  TestTypeVirtSolverHandler TTVSHandler;
  TBAAVirtSolverHandler TVSHandler;
  Walker.addHandler(&GISHandler);
  Walker.addHandler(&TTVSHandler);
  Walker.addHandler(&TSSHandler);
  Walker.addHandler(&TVSHandler);
  Walker.walk(M);
  TTVSHandler.collect(M);
  Solvers.emplace_back(
      std::make_unique<TestTypeVirtSolver>(std::move(TTVSHandler)));
  Solvers.emplace_back(
      std::make_unique<TBAASimpleSolver>(std::move(TSSHandler)));
  Solvers.emplace_back(
      std::make_unique<GlobalInitializerSolver>(std::move(GISHandler)));
  Solvers.emplace_back(std::make_unique<TBAAVirtSolver>(std::move(TVSHandler)));
}

const Function *IndCallSolverMgr::getCalledFunction(const CallBase &CB) const {
  auto Result = getCalledFunctions(CB);
  if (Result.size() == 0)
    return nullptr;

  return *Result.begin();
}

std::set<const Function *>
IndCallSolverMgr::getCalledFunctions(const CallBase &CB) const {
  // Step 1. check strip Pointer
  auto *CO = CB.getCalledOperand();
  if (!CO)
    return {};

  auto *V = CO->stripPointerCasts();
  if (!V)
    return {};

  if (V->getName() != "") {
    if (auto *F = dyn_cast<Function>(V))
      return {F};

    if (auto *GA = dyn_cast<GlobalAlias>(V)) {
      const auto *F = dyn_cast_or_null<Function>(GA->getAliasee());
      if (F)
        return {F};
    }

    return {};
  }

  // Step 2. check callee MD
  auto *Node = CB.getMetadata(LLVMContext::MD_callees);
  if (Node)
    return {mdconst::extract<Function>(Node->getOperand(0))};

  for (const auto &Solver : Solvers) {
    assert(Solver && "Unexpected Program State");

    auto Result = Solver->solve(CB);
    if (Result.size() > 0)
      return Result;
  }
  return {};
}
