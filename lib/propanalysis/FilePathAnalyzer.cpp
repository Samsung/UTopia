#include "FilePathAnalyzer.h"
#include "ftg/utils/LLVMUtil.h"
#include "ftg/utils/StringUtil.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace ftg {

FilePathAnalyzer::FilePathAnalyzer(IndCallSolverMgr *Solver,
                                   std::vector<const Function *> Funcs,
                                   const FilePathAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs) {
  if (Funcs.size() == 0)
    return;

  if (PreReport) {
    for (const auto &Iter : PreReport->get())
      Report.set(Iter.first, Iter.second);
  }

  analyze(Funcs);
}

std::unique_ptr<AnalyzerReport> FilePathAnalyzer::getReport() {
  return std::make_unique<FilePathAnalysisReport>(Report);
}

const FilePathAnalysisReport &FilePathAnalyzer::report() const {
  return Report;
}

void FilePathAnalyzer::analyzeProperty(Argument &A) {
  if (updateArgFlow(A))
    return;

  analyzeFilePath(A);
  Report.add(getOrCreateArgFlow(A));
}

void FilePathAnalyzer::analyzeFilePath(llvm::Argument &A) {
  std::set<Value *> VisitedNodes;
  ArgFlow &AF = getOrCreateArgFlow(A);
  std::stack<StackFrame> DefUseChains;
  DefUseChains.emplace(&A, AF);
  while (!DefUseChains.empty()) {
    auto Frame = DefUseChains.top();
    DefUseChains.pop();
    handleStackFrame(Frame, DefUseChains, VisitedNodes);
  }
}

void FilePathAnalyzer::handleStackFrame(StackFrame &Frame,
                                        std::stack<StackFrame> &DefUseChains,
                                        std::set<Value *> &VisitedNodes) {
  auto *Def = Frame.Value;
  assert(Def && "Unexpected Program State");

  if (VisitedNodes.find(Def) != VisitedNodes.end())
    return;
  if (isa<IntegerType>(Def->getType()))
    return;
  VisitedNodes.insert(Def);

  for (User *U : Def->users()) {
    if (!U)
      continue;
    handleUser(Frame, *U, DefUseChains, VisitedNodes);
  }
}

void FilePathAnalyzer::handleUser(StackFrame &Frame, Value &User,
                                  std::stack<StackFrame> &DefUseChains,
                                  std::set<Value *> &VisitedNodes) {
  Value *Def = Frame.Value;
  ArgFlow &DefFlow = Frame.AnalysisResult;
  auto &A = DefFlow.getLLVMArg();

  if (auto *SI = dyn_cast<StoreInst>(&User)) {
    auto *Op0 = SI->getOperand(0);
    auto *Op1 = SI->getOperand(1);

    if (VisitedNodes.find(Op1) == VisitedNodes.end()) {
      DefUseChains.emplace(Op1, DefFlow);
    }

    if ((VisitedNodes.find(Op0) == VisitedNodes.end()) && !isa<Constant>(Op0) &&
        !isa<GlobalValue>(Op0)) {
      DefUseChains.emplace(Op0, DefFlow);
    }
    return;
  }

  if (auto *CB = dyn_cast<CallBase>(&User)) {
    std::set<const Function *> CFs;
    if (Solver) {
      CFs = Solver->getCalledFunctions(*CB);
    } else {
      const auto *CF = util::getCalledFunction(*CB);
      if (CF)
        CFs.emplace(CF);
    }

    for (const auto *CF : CFs) {
      assert(CF && "Unexpected Program State");

      if (CF == A.getParent())
        continue;

      for (const auto &Param : CF->args()) {
        if (CB->arg_size() <= Param.getArgNo())
          continue;
        auto *CallArg = CB->getArgOperand(Param.getArgNo());
        if (CallArg != Def)
          continue;

        if (!CallArg)
          continue;

        analyze(Param);
        auto &CallAF = getOrCreateArgFlow(*const_cast<Argument *>(&Param));
        if (CallAF.isFilePathString()) {
          DefFlow.setFilePathString();
          return;
        }
      }

      auto Name = util::getDemangledName(CF->getName().str());
      auto Regex =
          util::regex(Name, "std::.*::basic_string.*::basic_string\\(char "
                            "const\\*, std::allocator<char> const&\\)");
      if (Regex == Name && CB->arg_size() == 3 &&
          CB->getArgOperand(1) == Def) {
        DefUseChains.emplace(CB->getArgOperand(0), DefFlow);
        continue;
      }

      Regex = util::regex(Name, "std::.*::basic_string.*::c_str const");
      if (Regex == Name && CB->arg_size() == 1) {
        const auto *Arg0 = CB->getArgOperand(0);
        assert(Def && "Unexpected Program State");
        assert(Arg0 && "Unexpected LLVM API Behavior");
        if (Def->getType() == Arg0->getType())
          DefUseChains.emplace(CB, DefFlow);
      }
    }
    return;
  }

  if (auto *I = dyn_cast<Instruction>(&User)) {
    if (I->isTerminator() || isa<CmpInst>(I))
      return;
    DefUseChains.emplace(&User, DefFlow);
  }
}

bool FilePathAnalyzer::updateArgFlow(Argument &A) {
  if (!Report.has(A))
    return false;

  auto &AF = getOrCreateArgFlow(A);
  if (Report.get(A))
    AF.setFilePathString();
  updateFieldFlow(AF);
  return true;
}

void FilePathAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  auto *T = A.getType();
  while (isa<PointerType>(T) && T->getNumContainedTypes() > 0) {
    T = T->getContainedType(0);
  }

  auto *ST = dyn_cast_or_null<StructType>(T);
  if (!ST)
    return;

  for (unsigned S = 0; S < ST->getNumElements(); ++S) {
    Indices.push_back(S);
    if (!Report.has(A, Indices)) {
      Indices.pop_back();
      continue;
    }

    AF.setStruct(ST);
    auto &FF = AF.getOrCreateFieldFlow(S);
    if (Report.get(A, Indices)) {
      FF.setFilePathString();
    }
    updateFieldFlow(FF, Indices);
    Indices.pop_back();
  }
}

} // namespace ftg
