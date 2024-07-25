#include "RDSpace.h"
#if LLVM_VERSION_MAJOR < 17
#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
#endif
#include "ftg/utils/LLVMUtil.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Passes/PassBuilder.h"
#include <queue>

using namespace llvm;

namespace ftg {

RDSpace::RDSpace() {
  PassBuilder PB;
  FAM.registerPass([&] { return MemorySSAAnalysis(); });
  FAM.registerPass([&] { return AAManager(); });
  PB.registerFunctionAnalyses(FAM);
}

void RDSpace::build(const std::vector<Function *> &Funcs) {
  // 1. Check Input Validity
  for (auto *F : Funcs) {
    if (!F)
      throw std::invalid_argument("Function should not be null");
    if (!F->getParent())
      throw std::invalid_argument("Function should be defined in a module");
  }

  // 2. Initialize Member Variables that should be rebuilt whenever build calls
  EntryFuncs = Funcs;
  LinkMap.clear();
  CallerMap.clear();

  // 3. Connect given Functions using Link Map
  for (size_t S = 1, E = EntryFuncs.size(); S < E; ++S)
    LinkMap.insert(std::make_pair(EntryFuncs[S], EntryFuncs[S - 1]));

  // 4. Build search space for every visitable function from given functions
  std::queue<Function *> Queue;
  std::set<Function *> VisitedFuncs;
  for (auto *F : EntryFuncs)
    Queue.push(F);
  while (Queue.size() > 0) {
    auto *Next = Queue.front();
    assert(Next && "Unexpected Program State");

    Queue.pop();
    if (VisitedFuncs.find(Next) != VisitedFuncs.end())
      continue;

    VisitedFuncs.insert(Next);
    auto DiscoveredFuncs = buildCallerMap(*Next);
    for (auto *F : DiscoveredFuncs)
      Queue.push(F);
    if (BuiltFuncs.find(Next) != BuiltFuncs.end())
      continue;

    build(*Next);
    BuiltFuncs.insert(Next);
  }
}

std::set<Instruction *> RDSpace::next(Instruction &I) {
  auto *F = I.getFunction();
  if (!F)
    return {};

  auto &SSA = FAM.getResult<MemorySSAAnalysis>(*F).getMSSA();
  auto *MA = SSA.getMemoryAccess(&I);
  if (!MA) {
    std::set<BasicBlock *> V;

    auto *F = I.getFunction();
    assert(F && "Unexpected Program State");

    return getLastMemInsts(*F, I, V);
  }

  std::set<Instruction *> Result;
  for (auto *NextMA : next(*MA)) {
    if (!NextMA)
      continue;

    for (auto *NextI : getInstructions(*NextMA)) {
      Result.insert(NextI);
    }
  }
  return Result;
}

std::set<llvm::Instruction *> RDSpace::nextInCallee(const llvm::Function &F,
                                                    bool Memory) {
  std::set<llvm::Instruction *> Result;

  if (Memory) {
    auto Iter = LeafMemMap.find(const_cast<llvm::Function *>(&F));
    if (Iter == LeafMemMap.end())
      return {};

    for (auto *I : Iter->second)
      Result.insert(I);
  } else {
    auto Iter = ReturnMap.find(const_cast<llvm::Function *>(&F));
    if (Iter == ReturnMap.end())
      return {};

    for (auto *I : Iter->second)
      Result.insert(I);
  }
  return Result;
}

std::set<llvm::Instruction *> RDSpace::nextInCaller(const llvm::Function &F) {
  std::set<llvm::Instruction *> Result;

  auto Iter = CallerMap.find(const_cast<llvm::Function *>(&F));
  if (Iter == CallerMap.end())
    return {};

  for (auto *CB : Iter->second)
    Result.insert(CB);

  return Result;
}

std::set<llvm::Instruction *> RDSpace::nextInLink(const llvm::Function &F) {
  auto Iter = LinkMap.find(const_cast<llvm::Function *>(&F));
  if (Iter == LinkMap.end())
    return {};

  assert(Iter->second && "Unexpected Program State");
  return nextInCallee(*Iter->second, true);
}

bool RDSpace::isEntryFunction(Function &F) const {
  return std::find(EntryFuncs.begin(), EntryFuncs.end(), &F) !=
         EntryFuncs.end();
}

std::set<Instruction *> RDSpace::getAliases(Instruction &I) const {
  auto Acc = AliasMap.find(&I);
  if (Acc == AliasMap.end())
    return {};

  return Acc->second;
}

bool RDSpace::dominates(Instruction &Dominator, Instruction &Dominatee) {
  if (Dominator.getFunction() != Dominatee.getFunction())
    return false;

  auto *F = Dominator.getFunction();
  auto *TorBB = Dominator.getParent();
  auto *TeeBB = Dominatee.getParent();
  assert((F && TorBB && TeeBB) && "Unexpected Program State");

  auto &DomTree = FAM.getResult<DominatorTreeAnalysis>(*F);

  if (TorBB == TeeBB)
    return DomTree.dominates(&Dominator, &Dominatee);

  auto *TorNode = DomTree.getNode(TorBB);
  auto *TeeNode = DomTree.getNode(TeeBB);
  assert((TorNode && TeeNode) && "Unexpected Program State");

  return DomTree.properlyDominates(TorNode, TeeNode);
}

std::set<Function *> RDSpace::buildCallerMap(Function &F) {
  std::set<Function *> Result;
  for (BasicBlock &B : F)
    for (Instruction &I : B) {
      auto *CB = dyn_cast_or_null<CallBase>(&I);
      if (!CB)
        continue;

      auto *CF = const_cast<Function *>(util::getCalledFunction(*CB));
      if (!CF)
        continue;

      updateMap(CallerMap, CF, CB);
      if (CF->size() == 0)
        continue;

      Result.insert(CF);
    }
  return Result;
}

void RDSpace::build(Function &F) {
  // 1. Traversing Instructions to get space information
  std::set<Function *> DiscoveredFuncs;
  std::vector<Instruction *> MemInsts;
  for (BasicBlock &B : F)
    for (Instruction &I : B) {
      if (isa<ReturnInst>(&I)) {
        // 1-1. Build Space about Function->Return relationship
        ReturnInst &RI = *dyn_cast<ReturnInst>(&I);
        Function *Parent = RI.getFunction();
        assert(Parent && "Unexpected Program State");

        updateMap(ReturnMap, Parent, &RI);
        continue;
      }

      if (isa<LoadInst>(&I) || isa<StoreInst>(&I)) {
        // 1-2. Collect Mem Insts to use alias analyses later
        MemInsts.push_back(&I);
      }
    }

  // 2. Build Space about Memory->Memory relationship(Pointers)
#if LLVM_VERSION_MAJOR < 17
  auto &AA = FAM.getResult<CFLAndersAA>(F);
#else
  AAResults &AA = FAM.getResult<AAManager>(F);
#endif
  for (size_t S1 = 0, E1 = MemInsts.size(); S1 < E1; ++S1) {
    assert(MemInsts[S1] && "Unexpected Program State");

    MemoryLocation MLoc1 = MemoryLocation::get(MemInsts[S1]);
    for (size_t S2 = 0, E2 = MemInsts.size(); S2 < E2; ++S2) {
      assert(MemInsts[S2] && "Unexpected Program State");

      if (MemInsts[S1] == MemInsts[S2])
        continue;

      MemoryLocation MLoc2 = MemoryLocation::get(MemInsts[S2]);
#if LLVM_VERSION_MAJOR < 17
      AAQueryInfo Info;
      if (AA.alias(MLoc1, MLoc2, Info) == AliasResult::MustAlias) {
#else
      if (AA.alias(MLoc1, MLoc2) == AliasResult::MustAlias) {
#endif
        updateMap(AliasMap, MemInsts[S1], MemInsts[S2]);
        updateMap(AliasMap, MemInsts[S2], MemInsts[S1]);
      }
    }
  }

  // 3. Build Space about Function->Leaf Memory Write Instruction relationship
  auto Acc = ReturnMap.find(&F);
  if (Acc != ReturnMap.end()) {
    for (auto *RetInst : Acc->second) {
      assert(RetInst && "Unexpected Program State");

      std::set<BasicBlock *> V = {RetInst->getParent()};
      for (auto *I : getLastMemInsts(F, *RetInst, V)) {
        updateMap(LeafMemMap, &F, I);
      }
    }
  }
}

std::set<llvm::Instruction *>
RDSpace::getLastMemInsts(Function &F, Instruction &I,
                         std::set<BasicBlock *> &V) {

  // 1. Get Memory SSA Analyses Result
  auto &SSA = FAM.getResult<MemorySSAAnalysis>(F).getMSSA();

  // 2. Get Memory Access Info about current instruction
  auto *MA = SSA.getMemoryAccess(&I);
  if (MA && !isa<MemoryUse>(MA)) {
    // 2-1. Connect Function -> Instructions that has memory access to write
    return getInstructions(*MA);
  }

  // 3. Not Found Memory Access. Continue to find (backtrace)
  Instruction *Prev = I.getPrevNode();
  if (Prev) {
    return getLastMemInsts(F, *Prev, V);
  }
  assert(I.getParent() && "Unexpected Program State");

  // 4. Go to Last Instruction of Previous Basic Block (backtrace)
  // Happens when previous instruction is an entry instruction of a basic block
  BasicBlock &B = *I.getParent();
  if (&B == &F.front())
    return {};
  // NOTE: Sometimes, there is a basic block does not have predecessor.
  // Still not understand why such basic blocks exist, anyway.
  if (pred_empty(&B))
    return {};

  std::set<BasicBlock *> NextBs;
  // 5. Find previous basic blocks using CFG in LLVM Analyses without Loop
  for (auto S = pred_begin(&B), E = pred_end(&B); S != E; ++S)
    if (V.find(*S) == V.end())
      NextBs.insert(*S);

  // 6. Sometimes, reaching entry basic block is impossible because
  // revisiting basic block is prevented. In this case, go to
  // immediate dominator of basic block.
  auto &Tree = FAM.getResult<DominatorTreeAnalysis>(F);
  auto *DomB = &B;
  while (NextBs.size() == 0) {
    auto DomNode = Tree.getNode(DomB);

    assert(DomNode && "Unexpected Program State");
    auto IDomNode = DomNode->getIDom();

    assert(IDomNode && "Unexpected Program State");
    DomB = IDomNode->getBlock();

    assert(DomB && "Unexpected Program State");
    if (V.find(DomB) == V.end())
      NextBs.insert(DomB);
  }
  assert(NextBs.size() > 0 && "Unexpected Program State");

  // 7. back-trace found basic block using CFG and dominator tree
  std::set<Instruction *> Result;
  for (auto nextB : NextBs) {
    V.insert(nextB);
    for (auto *Found : getLastMemInsts(F, nextB->back(), V)) {
      Result.insert(Found);
    }
    V.erase(nextB);
  }
  return Result;
}

template <typename KTy, typename VTy, typename MTy>
void RDSpace::updateMap(MTy &Map, KTy Key, VTy Val) {
  auto Acc = Map.find(Key);
  if (Acc == Map.end()) {
    Map.insert(std::make_pair(Key, std::set<VTy>({Val})));
    return;
  }

  Acc->second.insert(Val);
}

std::set<MemoryAccess *> RDSpace::next(MemoryAccess &MA) {
  std::set<MemoryAccess *> Result;
  if (isa<MemoryUseOrDef>(&MA)) {
    // 1. Get prev memory access
    MemoryAccess *PMA = dyn_cast<MemoryUseOrDef>(&MA)->getDefiningAccess();
    if (PMA)
      Result.insert(PMA);
    return Result;
  }
  assert(isa<MemoryPhi>(&MA) && "Unexpected Program State");

  // 2. Get memory access that memory phi indicates.
  MemoryPhi &MPhi = *dyn_cast<MemoryPhi>(&MA);
  for (size_t S = 0, E = MPhi.getNumIncomingValues(); S < E; ++S) {
    assert(MPhi.getIncomingValue(S) && "Unexpected Program State");

    for (auto PrevMA : next(*MPhi.getIncomingValue(S)))
      Result.insert(PrevMA);
  }
  return Result;
}

std::set<Instruction *>
RDSpace::getInstructions(MemoryAccess &MA, bool WithNull,
                         std::set<llvm::MemoryAccess *> Visit) {
  if (Visit.find(&MA) != Visit.end())
    return {};

  std::set<Instruction *> Result;
  if (isa<MemoryUseOrDef>(&MA)) {
    auto *I = dyn_cast<MemoryUseOrDef>(&MA)->getMemoryInst();
    if (I || WithNull)
      Result.insert(I);
    return Result;
  }
  assert(isa<MemoryPhi>(&MA) && "Unexpected Program State");

  MemoryPhi &MPhi = *dyn_cast<MemoryPhi>(&MA);
  for (size_t S = 0, E = MPhi.getNumIncomingValues(); S < E; ++S) {
    auto *NextMA = MPhi.getIncomingValue(S);
    assert(NextMA && "Unexpected Program State");

    Visit.insert(&MA);
    for (auto I : getInstructions(*NextMA, WithNull, Visit))
      Result.insert(I);
    Visit.erase(Visit.find(&MA));
  }
  return Result;
}

} // end namespace ftg
