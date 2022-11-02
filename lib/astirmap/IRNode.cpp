#include "ftg/astirmap/IRNode.h"
#include "ftg/utils/LLVMUtil.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/InstrTypes.h"

using namespace ftg;
using namespace llvm;

IRNode::IRNode(const llvm::Value &V) {
  if (const auto *I = dyn_cast<Instruction>(&V)) {
    Index = LocIndex::of(*I);
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

std::string IRNode::getFullPath(const DIGlobalVariable &G) const {
  auto *F = G.getFile();
  if (!F)
    return "";

  return util::getFullPath(*F);
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
