#include "ftg/astirmap/IRNode.h"
#include "ftg/utils/FileUtil.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstrTypes.h"

using namespace ftg;
using namespace llvm;

IRNode::IRNode(const llvm::Value &V) {
  if (const auto *A = dyn_cast<AllocaInst>(&V)) {
    setIndex(*A);
    return;
  }
  if (const auto *I = dyn_cast<Instruction>(&V)) {
    setIndex(*I);
    return;
  }
  if (const auto *G = dyn_cast<GlobalValue>(&V)) {
    setIndex(*G);
    return;
  }
  throw std::invalid_argument("Not Support LLVM Instance");
}

std::string IRNode::getName() const { return Name; }

const LocIndex &IRNode::getIndex() const { return Index; }

bool IRNode::operator<(const IRNode &Rhs) const {
  const auto &ThisIndex = getIndex();
  const auto &RhsIndex = Rhs.getIndex();
  if (ThisIndex != RhsIndex)
    return ThisIndex < RhsIndex;
  return Name < Rhs.getName();
}

raw_ostream &operator<<(raw_ostream &O, const IRNode &Src) {
  O << Src.getIndex().getIDAsString() << "\n";
  return O;
}

std::string IRNode::getFullPath(const DebugLoc &Loc) const {
  auto *DILoc = Loc.get();
  if (!DILoc)
    return "";

  auto *DIF = DILoc->getFile();
  if (!DIF)
    return "";

  return getFullPath(*DIF);
}

std::string IRNode::getFullPath(const DIGlobalVariable &G) const {
  auto *F = G.getFile();
  if (!F)
    return "";

  return getFullPath(*F);
}

std::string IRNode::getFullPath(const DIFile &F) const {
  std::string Name = F.getFilename();
  std::string Dir = F.getDirectory();
  std::string Result;

  if (Name.rfind(Dir, 0) == 0)
    Result = Name;
  else if (Dir.size() > 0 && Dir.back() == '/')
    Result = Dir + Name;
  else
    Result = Dir + '/' + Name;

  return util::getNormalizedPath(Result);
}

void IRNode::setIndex(const AllocaInst &I) {
  const auto *F = I.getFunction();
  assert(F && "Unexpected Program State");

  for (auto &IterB : *F)
    for (auto &IterI : IterB) {
      const auto *CB = dyn_cast<CallBase>(&IterI);
      if (!CB)
        continue;

      const auto *CF = CB->getCalledFunction();
      if (!CF)
        continue;
      if (!CF->isIntrinsic())
        continue;
      if (CF->getName() != "llvm.dbg.declare")
        continue;
      assert(CB->getNumArgOperands() > 0 && "Unexpected Program State");

      const auto *M1 = dyn_cast_or_null<MetadataAsValue>(CB->getArgOperand(0));
      if (!M1)
        continue;

      const auto *M2 = dyn_cast_or_null<LocalAsMetadata>(M1->getMetadata());
      if (!M2)
        continue;
      if (M2->getValue() != &I)
        continue;

      const auto &Loc = CB->getDebugLoc();
      Index = LocIndex(getFullPath(Loc), Loc.getLine(), Loc.getCol());
      break;
    }
}

void IRNode::setIndex(const GlobalValue &G) {
  if (const auto *GVariable = dyn_cast<GlobalVariable>(&G)) {
    SmallVector<DIGlobalVariableExpression *, 10> Debugs;
    GVariable->getDebugInfo(Debugs);

    for (auto *Debug : Debugs) {
      if (!Debug || !Debug->getVariable())
        continue;

      auto *Var = Debug->getVariable();
      Name = Var->getDisplayName();
      if (Name.empty())
        continue;

      Index = LocIndex(getFullPath(*Var), 0, 0);
      break;
    }
  }

  if (!Name.empty())
    return;

  Name = G.getName();
  assert(!Name.empty() && "Unexpected Program State");
}

void IRNode::setIndex(const Instruction &I) {
  const auto &Loc = I.getDebugLoc();
  Index = LocIndex(getFullPath(Loc), Loc.getLine(), Loc.getCol());
}
