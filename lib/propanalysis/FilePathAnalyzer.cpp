#include "FilePathAnalyzer.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

namespace ftg {

FilePathAnalyzer::FilePathAnalyzer(std::shared_ptr<IndCallSolver> Solver,
                                   std::vector<const Function *> Funcs,
                                   const FilePathAnalysisReport *PreReport)
    : ArgFlowAnalyzer(Solver, Funcs) {
  if (Funcs.size() == 0)
    return;

  const auto *F = Funcs[0];
  if (!F)
    return;

  const auto *M = F->getParent();
  if (!M)
    return;

  updateDefault(*M);

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
    auto *CF = getCalledFunction(*CB);
    if (!CF)
      return;
    if (CF == A.getParent())
      return;

    for (auto &Param : CF->args()) {
      auto *CallArg = CB->getArgOperand(Param.getArgNo());
      if (CallArg != Def)
        continue;
      if (!CallArg)
        continue;

      analyze(Param);
      auto &CallAF = getOrCreateArgFlow(Param);
      DefFlow.setFilePathString(CallAF.isFilePathString());
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
  AF.setFilePathString(Report.get(A));
  updateFieldFlow(AF);
  return true;
}

void FilePathAnalyzer::updateDefault(const Module &M) {
  const std::map<std::string, unsigned> DefaultParamMap = {
      {"access", 0},
      {"acct", 0},
      {"chdir", 0},
      {"creat", 0},
      {"chmod", 0},
      {"chown", 0},
      {"chroot", 0},
      {"execve", 0},
      {"execveat", 1},
      {"faccessat", 1},
      {"fanotify_mark", 4},
      {"fchmodat", 1},
      {"fchownat", 1},
      {"fopen", 0},
      {"futimesat", 1},
      {"getcwd", 0},
      {"getwd", 0},
      {"getxattr", 0},
      {"inotify_add_watch", 1},
      {"lchown", 0},
      {"lgetxattr", 0},
      {"link", 0},
      {"link", 1},
      {"linkat", 1},
      {"linkat", 3},
      {"listxattr", 0},
      {"llistxattr", 0},
      {"lookup_dcookie", 1},
      {"lremovexattr", 0},
      {"lsetxattr", 0},
      {"lstat", 0},
      {"memfd_create", 0},
      {"mkdir", 0},
      {"mkdirat", 1},
      {"mknod", 0},
      {"mknodat", 1},
      {"mount", 0},
      {"mount", 1},
      {"umount", 0},
      {"umount2", 0},
      {"name_to_handle_at", 1},
      {"open", 0},
      {"openat", 1},
      {"openat2", 1},
      {"pivot_root", 0},
      {"pivot_root", 1},
      {"readlink", 0},
      {"readlink", 1},
      {"readlinkat", 1},
      {"readlinkat", 2},
      {"realpath", 0},
      {"remove", 0},
      {"removexattr", 0},
      {"rename", 0},
      {"rename", 1},
      {"renameat", 1},
      {"renameat", 3},
      {"renameat2", 1},
      {"renameat2", 3},
      {"rmdir", 0},
      {"setxattr", 0},
      {"stat", 0},
      {"statfs", 0},
      {"swapoff", 0},
      {"swapon", 0},
      {"symlink", 0},
      {"symlink", 1},
      {"symlinkat", 0},
      {"symlinkat", 2},
      {"truncate", 0},
      {"unlink", 0},
      {"unlinkat", 1},
      {"utime", 0},
      {"utimensat", 1},
      {"utimes", 0}};

  for (const auto &F : M) {
    if (!F.isDeclaration())
      continue;

    auto FuncName = F.getName();
    auto Iter = DefaultParamMap.find(FuncName);
    if (Iter == DefaultParamMap.end())
      continue;

    updateDefault(F, Iter->second);
  }

  for (auto Iter : ArgFlowMap) {
    auto &AF = Iter.second;
    if (!AF)
      continue;

    if (AF->getState() != AnalysisState_Pre_Analyzed)
      continue;

    Report.add(*AF);
  }
}

void FilePathAnalyzer::updateDefault(const llvm::Function &F, unsigned ArgIdx) {
  for (const auto &Arg : F.args()) {
    bool FilePath = (Arg.getArgNo() == ArgIdx);
    auto &AF = getOrCreateArgFlow(*const_cast<Argument *>(&Arg));
    AF.setFilePathString(FilePath);
  }
}

void FilePathAnalyzer::updateFieldFlow(ArgFlow &AF, std::vector<int> Indices) {
  auto &A = AF.getLLVMArg();
  auto *T = A.getType();
  while (isa<PointerType>(T))
    T = T->getPointerElementType();

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
    FF.setFilePathString(Report.get(A, Indices));
    updateFieldFlow(FF, Indices);
    Indices.pop_back();
  }
}

} // namespace ftg
