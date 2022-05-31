#ifndef TESTS_PROPANALYSIS_TESTPROPANALYZER_HPP
#define TESTS_PROPANALYSIS_TESTPROPANALYZER_HPP

#include "TestHelper.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"

namespace ftg {

class TestPropAnalyzer : public testing::Test {

protected:
  std::vector<std::shared_ptr<CompileHelper>> CHs;
  std::unique_ptr<IRAccessHelper> IRAccess;
  llvm::FunctionAnalysisManager FAM;

  std::unique_ptr<IRAccessHelper> load(const std::string &CODE,
                                       CompileHelper::SourceType Type,
                                       std::string Name, std::string Option) {
    auto CH = TestHelperFactory().createCompileHelper(CODE, Name, Option, Type);
    if (!CH)
      return nullptr;

    auto *M = CH->getLLVMModule();
    if (!M)
      return nullptr;

    auto IRAccess = std::make_unique<IRAccessHelper>(*M);
    if (!IRAccess)
      return nullptr;

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

    FPM.addPass(llvm::PromotePass());
    FPM.addPass(llvm::SCCPPass());
    LPM.addPass(llvm::IndVarSimplifyPass());
    FPM.addPass(llvm::createFunctionToLoopPassAdaptor(std::move(LPM)));
    MPM.addPass(llvm::createModuleToFunctionPassAdaptor(std::move(FPM)));
    MPM.run(*M, MAM);

    CHs.emplace_back(CH);
    return IRAccess;
  }

  const llvm::Argument *getArg(IRAccessHelper &IRAccess, std::string FuncName,
                               unsigned ArgIndex) const {
    const auto *F = IRAccess.getFunction(FuncName);
    if (!F)
      return nullptr;

    return F->getArg(ArgIndex);
  }
};

} // namespace ftg

#endif // TESTS_PROPANALYSIS_TESTPROPANALYZER_HPP
