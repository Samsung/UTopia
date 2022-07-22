#include "ftg/utanalysis/TargetAPIFinder.h"
#include "ftg/utils/ASTUtil.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>

using namespace llvm;

namespace ftg {

TargetAPIFinder::TargetAPIFinder(std::set<Function *> APIs) : APIs(APIs) {}

std::set<Function *> TargetAPIFinder::findAPIs(std::vector<Function *> Funcs) {
  std::set<Function *> Result;
  find(Funcs, TargetCallees, Result);
  return Result;
}

std::set<CallBase *>
TargetAPIFinder::findAPICallers(std::vector<Function *> Funcs) {
  std::set<CallBase *> Result;
  find(Funcs, TargetCallers, Result);
  return Result;
}

template <typename T1, typename T2>
void TargetAPIFinder::find(std::vector<Function *> &Funcs, T1 &SrcMap,
                           T2 &Result) {
  std::queue<Function *> Queue;
  for (auto *Func : Funcs) {
    if (!Func)
      throw std::invalid_argument("Pointer is Null");
    Queue.push(Func);
  }

  std::set<Function *> Visited;
  while (Queue.size() > 0) {
    Function *Next = Queue.front();
    assert(Next && "Unexpected Program State");
    Queue.pop();

    if (Visited.find(Next) != Visited.end())
      continue;
    Visited.insert(Next);

    update(*Next);
    auto AccSrc = SrcMap.find(Next);
    assert(AccSrc != SrcMap.end() && "Unexpected Program State");
    Result.insert(AccSrc->second.begin(), AccSrc->second.end());

    auto AccSubroutines = Subroutines.find(Next);
    assert(AccSubroutines != Subroutines.end() && "Unexpected Program State");
    for (auto *Subroutine : AccSubroutines->second)
      Queue.push(Subroutine);
  }
}

void TargetAPIFinder::update(Function &Func) {
  std::set<llvm::Function *> CalleeSet;
  std::set<llvm::CallBase *> CallerSet;
  std::set<llvm::Function *> SubroutineSet;
  if (CachedFunctions.find(&Func) != CachedFunctions.end())
    return;

  for (BasicBlock &B : Func)
    for (Instruction &I : B) {
      auto *CB = dyn_cast_or_null<CallBase>(&I);
      if (!CB)
        continue;

      auto *CF = const_cast<Function *>(util::getCalledFunction(*CB));
      if (!CF)
        continue;
      if (APIs.find(CF) != APIs.end()) {
        CalleeSet.insert(CF);
        CallerSet.insert(CB);
        continue;
      }
      if (CF->size() == 0)
        continue;
      SubroutineSet.insert(CF);
    }
  TargetCallees.insert(std::make_pair(&Func, CalleeSet));
  TargetCallers.insert(std::make_pair(&Func, CallerSet));
  Subroutines.insert(std::make_pair(&Func, SubroutineSet));
  CachedFunctions.insert(&Func);
}

} // namespace ftg
