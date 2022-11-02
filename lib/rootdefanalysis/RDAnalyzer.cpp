#include "RDAnalyzer.h"
#include "ftg/utils/LLVMUtil.h"
#include "llvm/IR/Intrinsics.h"
#include <queue>

using namespace llvm;

namespace ftg {

RDAnalyzer::RDAnalyzer(unsigned Timeout, RDExtension *Extension)
    : Timeout(Timeout), Start() {
  if (Extension)
    this->Extension = *Extension;
}

void RDAnalyzer::setSearchSpace(const std::vector<Function *> &Functions) {
  Space.build(Functions);
}

std::set<RDNode> RDAnalyzer::getRootDefinitions(const llvm::Use &U) {
  auto *User = llvm::dyn_cast_or_null<llvm::Instruction>(U.getUser());
  if (!User)
    return {};

  Start = time(NULL);

  auto FoundDefs = findRootDefinitions(*const_cast<Use *>(&U));
  std::vector<RDNode> RootDefs(FoundDefs.begin(), FoundDefs.end());
  RootDefs.erase(std::remove_if(RootDefs.begin(), RootDefs.end(),
                                [](const auto &Node) {
                                  if (Node.isTimeout())
                                    return true;

                                  if (!Node.isRootDefinition()) {
                                    if (isa<RDMemory>(&Node.getTarget()))
                                      return false;

                                    return true;
                                  }

                                  return false;
                                }),
                 RootDefs.end());
  return std::set<RDNode>(RootDefs.begin(), RootDefs.end());
}

std::set<RDNode> RDAnalyzer::findRootDefinitions(Use &U) {
  unsigned OpIdx = U.getOperandNo();
  auto *I = dyn_cast_or_null<Instruction>(U.getUser());
  if (!I)
    return {};

  auto *F = I->getFunction();
  if (!F)
    return {};

  RDNode Node(OpIdx, *I);

  auto *CB = dyn_cast_or_null<CallBase>(I);
  if (!CB)
    return traverseFunction(Node, *F, true, false);

  auto *CF = CB->getCalledFunction();
  if (!CF)
    return traverseFunction(Node, *F, true, false);

  if (CF->isDeclaration())
    Node.setFirstUse(*CB, OpIdx);

  if (isForcedDef(Node)) {
    Node.setForcedDef();
    return {Node};
  }

  if (CF->isDeclaration()) {
    auto OutParamIndices = getOutParamIndices(*CB);

    if (OutParamIndices.find(OpIdx) == OutParamIndices.end()) {
      return traverseFunction(Node, *F, true, false);
    }

    std::set<RDNode> Result;
    for (size_t S = 0, E = CB->getNumArgOperands(); S < E; ++S) {
      if (OutParamIndices.find(S) != OutParamIndices.end())
        continue;

      for (auto &Trace : findRootDefinitions(CB->getOperandUse(S)))
        mergeRDNode(Result, Trace);
    }

    if (Result.size() == 0) {
      RDNode NewNode(OpIdx, *CB, &Node);
      NewNode.setForcedDef();
      mergeRDNode(Result, NewNode);
    }

    return Result;
  }

  return traverseFunction(Node, *F, true, false);
}

std::set<RDNode> RDAnalyzer::traverseFunction(RDNode &Node, Function &F,
                                              bool TraceCaller,
                                              bool TraceCallee) {
  auto &Location = Node.getLocation();
  std::set<RDNode> Prevs;
  if (!TraceCallee && isa<CallBase>(&Location)) {
    if (!Node.isVisit(Node.getTarget(), Location)) {
      Node.visit(Node.getTarget(), Location);
      Prevs = getPrevs(Node);
    }
  } else {
    Prevs.insert(Node);
  }

  std::set<RDNode> Result, Continue;
  if (Prevs.size() == 0) {
    Node.setIdx(-1);
    Continue.insert(Node);
  }

  for (const auto &Prev : Prevs) {
    for (const auto &NextNode : trace(*const_cast<RDNode *>(&Prev))) {
      if (NextNode.isRootDefinition()) {
        mergeRDNode(Result, NextNode);
        continue;
      }

      auto &Target = NextNode.getTarget().getIR();

      // NOTE: Alloca insts are considered as a candidate root definition
      // since uninitialized variable declarations are emitted to IR like this.
      if (isa<AllocaInst>(&Target)) {
        const_cast<RDNode *>(&NextNode)->setIdx(-1);
        mergeRDNode(Result, NextNode);
        continue;
      }

      // NOTE: Only global variable and argument can be continued outside
      // of this function.
      if (!isa<GlobalVariable>(&Target) && !isa<Argument>(&Target))
        continue;

      RDNode NewNode(NextNode);
      NewNode.setStopTracing(false);
      mergeRDNode(Continue, NewNode);
    }
  }

  if (TraceCaller) {
    if (!Space.isEntryFunction(F)) {
      while (Continue.size() > 0) {
        auto Iter = Continue.begin();
        mergeRDNodes(Result, traceCaller(*const_cast<RDNode *>(&*Iter), F));
        Continue.erase(Iter);
      }
    } else {
      while (Continue.size() > 0) {
        auto Iter = Continue.begin();
        mergeRDNodes(Result, traceLink(*const_cast<RDNode *>(&*Iter), F));
        Continue.erase(Iter);
      }
    }
  }
  mergeRDNodes(Result, Continue);

  // NOTE: Restore previous node visit information to keep visit context
  // to traverse outside of this function.
  for (const auto &ResultNode : Result)
    const_cast<RDNode *>(&ResultNode)->copyVisit(Node);
  return Result;
}

std::set<RDNode> RDAnalyzer::next(RDNode &Node) {
  if (Node.isMemory()) {
    if (auto *SI = dyn_cast<StoreInst>(&Node.getLocation()))
      return handleStoreInst(Node, *SI);

    if (auto *CB = dyn_cast<CallBase>(&Node.getLocation()))
      return handleMemoryCall(Node, *CB);
  }
  return {Node};
}

std::set<RDNode> RDAnalyzer::getPrevs(RDNode &Node) {

  auto &Target = Node.getTarget();
  std::set<RDNode> Result;

  if (isa<RDConstant>(&Target))
    Result = {Node};
  if (isa<RDMemory>(&Target))
    Result = getPrevsMemoryBase(Node);
  if (isa<RDRegister>(&Target))
    Result = getPrevsRegBase(Node);

  return Result;
}

std::set<RDNode> RDAnalyzer::getPrevsMemoryBase(RDNode &Node) {
  auto &Target = Node.getTarget().getIR();
  std::set<RDNode> Result;
  for (auto *I : Space.next(Node.getLocation())) {
    if (isa<AllocaInst>(&Target)) {
      // Memory SSA does not care alloca instruction.
      // Thus get prevs should stopped if found memory related instruction
      // does not dominates alloca instruction.
      auto &AI = *dyn_cast<AllocaInst>(&Target);
      if (!Space.dominates(AI, *I))
        continue;
    }

    RDNode NewNode(Node);
    NewNode.setLocation(*I);
    Result.insert(NewNode);
  }
  return Result;
}

std::set<RDNode> RDAnalyzer::getPrevsRegBase(RDNode &Node) {
  auto &Target = Node.getTarget().getIR();
  assert(isa<Instruction>(&Target) && "Unexpected Program State");

  auto &I = *cast<Instruction>(&Target);
  if (isa<CallBase>(&I)) {
    std::set<RDNode> Result;
    for (const auto &Node : handleRegister(Node, *cast<CallBase>(&I)))
      mergeRDNodes(Result, pass(*const_cast<RDNode *>(&Node)));
    return Result;
  }

  std::set<RDNode> Result;
  for (size_t S = 0, E = I.getNumOperands(); S < E; ++S) {
    auto *Arg = I.getOperand(S);
    assert(Arg && "Unexpected Program State");

    Arg = Arg->stripPointerCasts();
    assert(Arg && "Unexpected Program State");

    if (isa<Constant>(Arg) && !isa<GlobalVariable>(Arg)) {
      // 1) A constant in a register is not needed to be traced.
      // 2) A constant in a register is not a valid definition.
      continue;
    }
    Result.emplace(S, I, &Node);
  }
  return Result;
}

std::set<RDNode> RDAnalyzer::trace(RDNode &Node) {
  if (Node.isStopTracing())
    return {Node};

  if (Cache.has(Node)) {
    auto CachedNodes = Cache.get(Node);
    for (const auto &CachedNode : CachedNodes) {
      const_cast<RDNode *>(&CachedNode)->copyVisit(Node);
      if (CachedNode.getFirstUses().size() == 0) {
        const_cast<RDNode *>(&CachedNode)->setFirstUses(Node.getFirstUses());
      }
    }

    return CachedNodes;
  }

  if (Node.isRootDefinition()) {
    auto *CB = dyn_cast_or_null<CallBase>(&Node.getLocation());
    auto Idx = Node.getIdx();
    if (CB && Idx >= 0)
      Node.setFirstUse(*CB, Idx);
    return {Node};
  }

  auto &Location = Node.getLocation();
  if (Node.isVisit(Node.getTarget(), Location)) {
    return {};
  }

  if (Timeout != 0 && static_cast<unsigned>(time(NULL) - Start) > Timeout) {
    Node.timeout();
    return {Node};
  }

  RDNode NewNode = Node;
  NewNode.clearFirstUses();
  NewNode.visit(Node.getTarget(), Location);

  std::set<RDNode> Result;
  for (const auto &NextNode : next(NewNode))
    mergeRDNodes(Result, pass(*const_cast<RDNode *>(&NextNode)));

  // If there is timeout result, entire result is not cached.
  for (auto &Node : Result)
    if (Node.isTimeout())
      return Result;

  if (Result.size() > 0) {
    // Build cache.
    Cache.cache(NewNode, Result);

    // Prepare result. Use first use if found node has not first use.
    // This logic should be done after build cache, because built cache
    // at this point can not use first uses that were found previously.
    for (auto &FoundNode : Result) {
      auto &FNFUs = FoundNode.getFirstUses();

      if ((FNFUs.size() > 0) && (FNFUs.find(RDArgIndex()) == FNFUs.end()))
        continue;

      // Use previous first use if
      //   (1) First use not found.
      //   (2) First uses has NONE that means there is a path where
      //       first use is not set.
      auto &NFUs = Node.getFirstUses();
      auto &FN = *const_cast<RDNode *>(&FoundNode);
      FN.addFirstUses(NFUs);
      FN.eraseEmptyFirstUse();
    }
  }

  return Result;
}

std::set<RDNode> RDAnalyzer::traceCaller(RDNode &Node, llvm::Function &F) {

  auto &Target = Node.getTarget().getIR();
  if (!isa<GlobalVariable>(&Target) && !isa<Argument>(&Target)) {
    return {Node};
  }

  std::set<RDNode> Result;
  for (auto *Caller : Space.nextInCaller(F)) {
    if (isa<GlobalVariable>(&Target)) {
      RDNode NewNode(Target, *Caller, &Node);
      for (auto &Found :
           traverseFunction(NewNode, *Caller->getFunction(), true, false))
        mergeRDNode(Result, Found);
      continue;
    }

    if (isa<Argument>(&Target)) {
      auto &Arg = *dyn_cast<Argument>(&Target);
      assert(Arg.getParent() == &F && "Unexpected Program State");

      RDNode NewNode(Arg.getArgNo(), *Caller, &Node);

      for (auto &Found :
           traverseFunction(NewNode, *Caller->getFunction(), true, false))
        mergeRDNode(Result, Found);
      continue;
    }

    assert(false && "Unexpected Program State");
  }

  return Result;
}

std::set<RDNode> RDAnalyzer::traceLink(RDNode &Node, Function &F) {
  auto &Target = Node.getTarget().getIR();
  if (!isa<GlobalVariable>(&Target))
    return {Node};

  std::set<RDNode> Result;
  auto LeafMems = Space.nextInLink(F);

  if (LeafMems.size() == 0) {
    mergeRDNode(Result, Node);
    return Result;
  }
  for (auto *Caller : LeafMems) {
    RDNode NewNode(Node.getTarget().getIR(), *Caller, &Node);
    for (auto &Found :
         traverseFunction(NewNode, *Caller->getFunction(), true, true))
      mergeRDNode(Result, Found);
  }

  return Result;
}

std::set<RDNode> RDAnalyzer::pass(RDNode &Node) {
  if (Node.isRootDefinition())
    return {Node};

  auto Prevs = getPrevs(Node);
  // There is no path to explore, mark it not to futher tracing.
  if (Prevs.empty()) {
    RDNode NewNode(Node);
    NewNode.setStopTracing(true);
    return {NewNode};
  }

  std::set<RDNode> Result;
  for (auto &Prev : Prevs) {
    for (auto &Found : trace(*const_cast<RDNode *>(&Prev))) {
      mergeRDNode(Result, Found);
    }
  }
  return Result;
}

std::set<RDNode> RDAnalyzer::handleRegister(RDNode &Node, CallBase &CB) {
  const auto *CF = util::getCalledFunction(CB);
  bool Revisited = CF ? Node.isVisit(*CF) : false;
  auto Result = handleRegisterInternal(Node, CB);
  if (!Revisited)
    assert(!Result.empty() && "Unexpected Program States");
  return Result;
}

std::set<RDNode> RDAnalyzer::handleRegisterInternal(RDNode &Node,
                                                    CallBase &CB) {
  auto *F = CB.getCalledFunction();
  if (!F)
    return {Node};
  assert(F->getReturnType() && !F->getReturnType()->isVoidTy() &&
         "Unexpected Program State");

  auto &Target = Node.getTarget().getIR();
  assert(isa<Instruction>(&Target) && "Unexpected Program State");

  std::set<RDNode> Result;
  if (F->isDeclaration()) {
    return handleExternalCall(Node, CB);
  }

  auto Returns = Space.nextInCallee(*F);
  assert(Returns.size() > 0 && "Unexpected Program State");

  for (auto *Return : Returns) {
    assert(Return && "Unexpected Program State");

    RDNode NewNode(0, *Return, &Node);
    std::set<RDNode> Continues;
    for (auto &Found : traverseFunction(NewNode, *F, false, true)) {

      auto &FoundTarget = const_cast<RDNode *>(&Found)->getTarget().getIR();
      if (isa<GlobalVariable>(&FoundTarget)) {
        Continues.emplace(FoundTarget, CB, &Found);
        continue;
      }

      if (isa<Argument>(&FoundTarget)) {
        auto &Arg = *dyn_cast<Argument>(&FoundTarget);
        size_t ArgNo = Arg.getArgNo();
        Continues.emplace(ArgNo, CB, &Found);
        continue;
      }

      Result.insert(Found);
    }

    for (auto &Next : Continues) {
      auto &TNext = *const_cast<RDNode *>(&Next);
      if (isForcedDef(TNext))
        TNext.setForcedDef();

      Result.insert(TNext);
    }
  }
  return Result;
}

std::set<RDNode> RDAnalyzer::handleStoreInst(RDNode &Node, StoreInst &I) {
  auto *Ptr = I.getPointerOperand();
  assert(Ptr && "Unexpected Program State");

  std::set<RDNode> Result;
  auto PropRet = isPropagated(Node.getTarget(), *Ptr);
  if (PropRet == PROPTYPE_NONE)
    return {Node};

  RDNode NewNode(0, I, &Node);
  // NOTE: Target of register should be same as its location.
  if (isa<RDRegister>(NewNode.getTarget())) {
    auto *LocI = dyn_cast<Instruction>(&NewNode.getTarget().getIR());
    assert(LocI && "Unexpected Program State");
    NewNode.setLocation(*LocI);
  }

  if (PropRet == PROPTYPE_INCL)
    return {NewNode};

  return {Node, NewNode};
}

std::set<RDNode> RDAnalyzer::handleMemoryCall(RDNode &Node, CallBase &CB) {
  auto *F = CB.getCalledFunction();
  if (!F)
    return {Node};

  if (F->isDeclaration()) {
    return handleExternalCall(Node, CB);
  }

  auto LeafMems = Space.nextInCallee(*F, true);
  if (LeafMems.size() == 0)
    return {Node};

  std::set<llvm::Value *> Cands;
  const auto &Target = Node.getTarget();

  if (Target.isGlobalVariable())
    Cands.insert(&Target.getIR());

  // CallBase Argument size always bigger than Function parameter size
  // because variable argument functions aceept additional arguments.
  for (unsigned S = 0, E = F->arg_size(); S < E; ++S) {
    auto *Arg = CB.getArgOperand(S);
    assert(Arg && "Unexpected Program State");

    auto *ArgType = Arg->getType();
    assert(ArgType && "Unexpected Program State");

    if (!ArgType->isPointerTy() || isPropagated(Target, *Arg) == PROPTYPE_NONE)
      continue;

    Cands.insert(F->getArg(S));
  }

  if (Cands.size() == 0)
    return {Node};

  bool NullFound = false;
  std::set<RDNode> Continues;
  std::set<RDNode> Result;

  for (auto LeafMem : LeafMems) {
    if (!LeafMem) {
      NullFound = true;
      continue;
    }

    std::set<RDNode> Founds;
    for (auto Cand : Cands) {
      RDNode NewNode(*Cand, *LeafMem, &Node);
      for (auto &Found : traverseFunction(NewNode, *F, false, true)) {
        if (&Found.getTarget().getIR() == Cand) {
          NullFound = true;
          continue;
        }
        Founds.insert(Found);
      }
    }

    while (Founds.size() > 0) {
      RDNode Found(*Founds.begin());
      Founds.erase(Founds.begin());
      auto &FoundTarget = Found.getTarget().getIR();

      if (Found.isRootDefinition()) {
        mergeRDNode(Result, Found);
        continue;
      }

      if (isa<AllocaInst>(&FoundTarget)) {
        RDNode NewNode(Found);
        NewNode.setForcedDef();
        mergeRDNode(Result, Found);
        continue;
      }

      if (isa<Argument>(&FoundTarget)) {
        Argument &FuncArg = *dyn_cast<Argument>(&FoundTarget);
        size_t ArgNo = FuncArg.getArgNo();
        auto *Arg = CB.getArgOperand(ArgNo);
        assert(Arg && "Unexpected Program State");

        mergeRDNode(Continues, RDNode(*Arg, CB, &Node));
        continue;
      }

      if (isa<GlobalVariable>(&FoundTarget)) {
        mergeRDNode(Continues, RDNode(FoundTarget, CB, &Node));
        continue;
      }

      assert(false && "Unexpected Program State");
    }
  }

  for (auto &Continue : Continues) {
    auto &PrevNode = *const_cast<RDNode *>(&Continue);

    // NOTE: Not necessary to set FirstUse here,
    //       because NewNode traces new definition. Unfortunately, first use
    //       is updated automatically if there is no first use, thus
    //       adding dummy first use is required.
    RDNode NewNode(PrevNode.getTarget(), CB, &Node);
    NewNode.setFirstUse(CB, NewNode.getIdx());
    Result.insert(NewNode);
  }

  if (NullFound)
    Result.insert(Node);

  return Result;
}

std::set<RDNode> RDAnalyzer::handleExternalCall(RDNode &Node, CallBase &CB) {
  // 1. Checks whether a given RDNode is register or memory
  std::set<RDNode> Result;
  if (Node.isMemory()) {
    if (isMemoryIntrinsic(CB))
      return handleMemoryIntrinsicCall(Node, CB);

    // In memory case (ex: ... = external call(..., target, ...))
    // If trace target is a parameter of external call and the parameter
    // is known as the one whose direction is output, then stop tracing.
    // If not, keep tracing.

    // 1-1-1. Collect parameters whose direction is output.
    auto OutParamIndices = getOutParamIndices(CB);

    if (isTargetMethodInvocation(Node, CB)) {
      auto NonOutParamIndices = getNonOutParamIndices(CB);

      std::set<RDNode> NextNodes = {Node};
      for (auto NonOutParamIndex : NonOutParamIndices) {
        RDNode NextNode(NonOutParamIndex, CB, &Node);
        NextNode.setFirstUse(CB, NonOutParamIndex);
        NextNodes.insert(NextNode);
      }
      return NextNodes;
    }

    // 1-1-2. Collect tracing target is in out parameters.
    const auto &Target = Node.getTarget();
    bool SetFirstUse = false;
    std::vector<size_t> MatchedParamIndices;
    for (size_t S = 0, E = CB.getNumArgOperands(); S < E; ++S) {
      auto *Arg = CB.getArgOperand(S);
      assert(Arg && "Unexpected Program State");

      if (isPropagated(Target, *Arg) == PROPTYPE_NONE)
        continue;

      if (Target == *RDTarget::create(*Arg)) {
        if (SetFirstUse)
          Node.addFirstUse(CB, S);
        else {
          SetFirstUse = true;
          Node.setFirstUse(CB, S);
        }
      }

      if (OutParamIndices.find(S) == OutParamIndices.end())
        continue;

      MatchedParamIndices.push_back(S);
    }

    // 1-1-3. If tracing target is not an output parameter, go next.
    if (MatchedParamIndices.empty()) {
      return {Node};
    }

    // 1-1-4. Getting tracing result of other arguments and use their results
    // as the result of out parameters.
    for (size_t S = 0, E = CB.getNumArgOperands(); S < E; ++S) {
      if (OutParamIndices.find(S) != OutParamIndices.end())
        continue;

      if (Extension.isTermination(CB, S)) {
        RDNode NewNode(S, CB, &Node);
        NewNode.setForcedDef();
        NewNode.setFirstUse(CB, S);
        Result.insert(NewNode);
        continue;
      }

      RDNode NewNode(S, CB, &Node);
      NewNode.setFirstUse(CB, S);
      if (NewNode.isRootDefinition()) {
        NewNode.setForcedDef();
        Result.insert(NewNode);
        continue;
      }

      Result.insert(NewNode);
    }

    // 1-1-5. If no result, return output parameter itself as a root definition.
    if (Result.empty()) {
      for (auto &ParamIndex : MatchedParamIndices) {
        RDNode NewNode(ParamIndex, CB, &Node);
        NewNode.setForcedDef();
        NewNode.setFirstUse(CB, ParamIndex);
        Result.insert(NewNode);
      }
    }

    return Result;
  } else {
    // 1-2. In register case (ex: target = external call(...).
    // Trace its parameter instead (Policy)

    // 1-2-1. Collect parameters whose direction is output.
    auto OutParamIndices = getOutParamIndices(CB);

    for (size_t S = 0, E = CB.getNumArgOperands(); S < E; ++S) {
      // 1-2-2-1. Insert Output Parameter.
      if (OutParamIndices.find(S) != OutParamIndices.end())
        continue;

      // 1-2-2-2. Make RDNode for its argument.
      RDNode NewNode(S, CB, &Node);
      NewNode.setFirstUse(CB, S);

      // 1-2-2-3. Not Trace if it is a root definition
      if (Extension.isTermination(CB, S)) {
        NewNode.setForcedDef();
        Result.insert(NewNode);
      }

      if (NewNode.isRootDefinition()) {
        Result.insert(NewNode);
        continue;
      }

      // 1-2-2-4. Getting tracing result of other arguments and use their
      // results as the result of out parameters.
      Result.insert(NewNode);
    }

    if (Result.size() == 0) {
      // 1-3. Return this node as an output.
      RDNode NewNode(Node);
      NewNode.setForcedDef();
      NewNode.setFirstUses(Node.getFirstUses());
      Result.insert(NewNode);
    }
  }
  return Result;
}

std::set<RDNode> RDAnalyzer::handleMemoryIntrinsicCall(RDNode &Node,
                                                       CallBase &CB) {
  auto *F = CB.getCalledFunction();
  assert((F && F->isIntrinsic()) && "Unexpected Program State");

  auto IntrinsicID = F->getIntrinsicID();
  assert((IntrinsicID == Intrinsic::memcpy ||
          IntrinsicID == Intrinsic::memcpy_element_unordered_atomic ||
          IntrinsicID == Intrinsic::memset ||
          IntrinsicID != Intrinsic::memset_element_unordered_atomic) &&
         "Unexpected Program State");

  auto Dst = RDTarget::create(*CB.getOperand(0));
  auto &NodeTarget = Node.getTarget();
  if (Dst->includes(NodeTarget)) {
    RDNode Next(Node);
    Next.setTarget(*CB.getOperand(1));

    if (Next.isRootDefinition())
      Next.setIdx(-1);

    return {Next};
  }

  if (NodeTarget.includes(*Dst)) {
    RDNode Next(Node);
    Next.setTarget(*CB.getOperand(1));
    if (Next.isRootDefinition())
      Next.setIdx(-1);

    return {Next, Node};
  }

  return {Node};
}

bool RDAnalyzer::isOutParam(const llvm::CallBase &CB, size_t ArgIdx) const {
  const auto *F = CB.getCalledFunction();
  if (!F || !F->isDeclaration())
    return false;
  if (CB.hasStructRetAttr() && ArgIdx == 0)
    return true;

  if (ArgIdx >= F->arg_size())
    return false;

  const auto *A = F->getArg(ArgIdx);
  if (!A)
    return false;

  return Extension.isOutParam(*A);
}

std::set<size_t> RDAnalyzer::getOutParamIndices(CallBase &CB) const {
  std::set<size_t> Result;

  // 1. Checks program states.
  auto *F = CB.getCalledFunction();
  assert((F && F->isDeclaration()) && "Unexpected Program State");

  // 2. Checks this call has sret attribute.
  unsigned S = 0;
  bool HasStructRet = CB.hasStructRetAttr();
  if (HasStructRet) {
    Result.insert(0);
    S = 1;
  }

  // 3. Collect parameter indices whose direction is output.
  for (auto E = CB.getNumArgOperands(); S < E; ++S) {
    if (isOutParam(CB, S))
      Result.insert(S);
  }
  return Result;
}

std::set<unsigned> RDAnalyzer::getNonOutParamIndices(const CallBase &CB) const {

  unsigned StartIndex = 0;
  if (CB.hasStructRetAttr())
    StartIndex += 1;
  if (isNonStaticMethodInvocation(CB))
    StartIndex += 1;

  unsigned EndIndex = CB.getNumArgOperands();

  std::set<unsigned> NonOutParamIndices;

  for (auto S = StartIndex; S < EndIndex; ++S) {
    if (isOutParam(CB, S))
      continue;
    NonOutParamIndices.insert(S);
  }

  return NonOutParamIndices;
}

bool RDAnalyzer::isForcedDef(RDNode &Node) const {
  auto &Location = Node.getLocation();
  auto *CB = dyn_cast_or_null<CallBase>(&Location);
  if (!CB)
    return false;

  auto Idx = Node.getIdx();
  if (Idx < 0)
    return false;
  assert((size_t)Idx < CB->getNumArgOperands() && "Unexpected Program State");

  return Extension.isTermination(*CB, Idx);
}

bool RDAnalyzer::terminates(RDNode &Node) const {
  if (Node.isRootDefinition())
    return true;

  auto &Location = Node.getLocation();
  auto *CB = dyn_cast_or_null<CallBase>(&Location);
  if (!CB)
    return false;

  auto Idx = Node.getIdx();
  if (Idx < 0)
    return false;
  assert((size_t)Idx < CB->getNumArgOperands() && "Unexpected Program State");

  return Extension.isTermination(*CB, Idx);
}

bool RDAnalyzer::isMemoryIntrinsic(llvm::CallBase &CB) const {

  auto *F = CB.getCalledFunction();
  if (!F || !F->isIntrinsic())
    return false;

  auto IntrinsicID = F->getIntrinsicID();
  if (IntrinsicID != Intrinsic::memcpy &&
      IntrinsicID != Intrinsic::memcpy_element_unordered_atomic &&
      IntrinsicID != Intrinsic::memset &&
      IntrinsicID != Intrinsic::memset_element_unordered_atomic)
    return false;

  return true;
}

bool RDAnalyzer::isTargetMethodInvocation(const RDNode &Node,
                                          const llvm::CallBase &CB) {

  // CallBase should call non static method invocation.
  if (!isNonStaticMethodInvocation(CB))
    return false;

  // Getting an argument index of the instance of a class.
  unsigned ClsIndex = 0;
  if (CB.hasStructRetAttr())
    ClsIndex += 1;
  auto *Arg = CB.getArgOperand(ClsIndex);

  // Checks the instance of a class is same as the tracing target.
  auto Targets = RDTarget::create(*Arg, &Space);
  if (std::find_if(Targets.begin(), Targets.end(), [&Node](const auto &Src) {
        return Node.getTarget() == *Src;
      }) == Targets.end())
    return false;

  return true;
}

bool RDAnalyzer::isNonStaticMethodInvocation(const llvm::CallBase &CB) const {

  auto *F = CB.getCalledFunction();
  if (!F) {
    // Return false unless which function is called by this instruction.
    return false;
  }

  auto Name = F->getName();
  return Extension.isNonStaticClassMethod(Name.str());
}

RDAnalyzer::PROPTYPE RDAnalyzer::isPropagated(const RDTarget &Target,
                                              llvm::Value &Src) {

  RDAnalyzer::PROPTYPE Result = PROPTYPE_NONE;
  for (auto &Cand : RDTarget::create(Src, &Space)) {
    if (Cand->includes(Target))
      return PROPTYPE_INCL;
    if (Target.includes(*Cand))
      Result = PROPTYPE_PART;
  }

  return Result;
}

void RDAnalyzer::mergeRDNodes(std::set<RDNode> &Dst,
                              std::set<RDNode> Srcs) const {
  for (const auto &Src : Srcs)
    mergeRDNode(Dst, Src);
}

void RDAnalyzer::mergeRDNode(std::set<RDNode> &Dst, const RDNode &Src) const {
  auto Acc = Dst.find(Src);
  if (Acc == Dst.end()) {
    Dst.insert(Src);
    return;
  }

  auto &Node = *const_cast<RDNode *>(&*Acc);
  auto &SrcFUs = Src.getFirstUses();

  if (SrcFUs.size() == 0) {
    if (Node.getFirstUses().size() > 0) {
      Node.addFirstUse(RDArgIndex());
    }
    return;
  }

  for (const auto &FirstUse : Src.getFirstUses()) {
    Node.addFirstUse(FirstUse);
  }
}

} // end namespace ftg
