#include "RDNode.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"

using namespace llvm;

namespace ftg {

RDNode::RDNode(int Idx, Instruction &I, const RDNode *Before) : Timeout(false) {

  this->ForcedDef = false;

  if (Before)
    *this = *Before;

  unsigned MaxIdx = 0;
  if (auto *CB = dyn_cast_or_null<CallBase>(&I))
    MaxIdx = CB->getNumArgOperands();
  else
    MaxIdx = I.getNumOperands();

  if ((unsigned)Idx >= MaxIdx)
    throw std::invalid_argument("Out of Index");

  this->Idx = Idx;
  this->Target = RDTarget::create(*I.getOperand(Idx));
  this->Location = &I;
}

RDNode::RDNode(Value &V, Instruction &I, const RDNode *Before)
    : Timeout(false) {

  this->ForcedDef = false;

  if (Before)
    *this = *Before;

  this->Idx = getIdx(V, I);
  this->Target = RDTarget::create(V);
  this->Location = &I;
}

RDNode::RDNode(const RDTarget &Target, llvm::Instruction &I,
               const RDNode *Before)
    : ForcedDef(false), Timeout(false) {

  if (Before)
    *this = *Before;

  this->Idx = getIdx(Target, I);
  this->Target = RDTarget::create(Target);
  this->Location = &I;
}

RDNode::RDNode(const RDNode &Src) { *this = Src; }

void RDNode::setIdx(int Idx) { this->Idx = Idx; }

void RDNode::setTarget(llvm::Value &V) {

  Target = RDTarget::create(V);
  Idx = getIdx(*Target, *Location);
}

void RDNode::setLocation(llvm::Instruction &I) {

  Location = &I;
  Idx = getIdx(*Target, *Location);
}

void RDNode::setVisit(
    const std::map<Function *, std::set<Instruction *>> &Visit) {

  this->Visit = Visit;
}

void RDNode::setFirstUses(const std::set<RDArgIndex> &FirstUses) {

  this->FirstUses = FirstUses;
}

void RDNode::setFirstUse(llvm::CallBase &CB, int ArgNo) {

  auto *CF = CB.getCalledFunction();
  if (CF && CF->isDeclaration()) {
    FirstUses.clear();
    addFirstUse(CB, ArgNo);
  }
}

void RDNode::setForcedDef() { this->ForcedDef = true; }

void RDNode::timeout() {

  this->Timeout = true;
  setForcedDef();
}

void RDNode::visit(Instruction &I) {

  auto *F = I.getFunction();
  assert(F && "Unexpected Program State");

  auto Iter = Visit.find(F);
  if (Iter != Visit.end()) {
    if (!Iter->second.insert(&I).second) {
      assert(false && "Unexpected Program State");
    }
    return;
  }

  std::set<Instruction *> VIs = {&I};
  Visit.emplace(F, VIs);
}

void RDNode::addFirstUse(llvm::CallBase &CB, int ArgNo) {

  FirstUses.emplace(CB, ArgNo);
}

void RDNode::addFirstUse(const RDArgIndex &Src) { FirstUses.insert(Src); }

void RDNode::addFirstUses(const std::set<RDArgIndex> &FirstUses) {

  for (auto &FU : FirstUses)
    addFirstUse(FU);
}

void RDNode::cancelVisit(Function &F) { Visit.erase(&F); }

void RDNode::clearVisit() { Visit.clear(); }

void RDNode::eraseEmptyFirstUse() { FirstUses.erase(RDArgIndex()); }

void RDNode::clearFirstUses() { FirstUses.clear(); }

int RDNode::getIdx() const { return Idx; }

const RDTarget &RDNode::getTarget() const {

  assert(Target && "Unexpected Program State");
  return *Target;
}

llvm::Instruction &RDNode::getLocation() {

  assert(Location && "Unexpected Program State");
  return *Location;
}

const std::map<Function *, std::set<Instruction *>> &RDNode::getVisit() const {

  return Visit;
}

const std::set<RDArgIndex> &RDNode::getFirstUses() const { return FirstUses; }

std::pair<Value *, int> RDNode::getDefinition() const {

  assert(Target && Location && "Unexpected Program State");

  if (isa<CallBase>(Location) && Idx >= 0) {
    return std::make_pair(Location, Idx);
  }

  assert(Target && "Unexpected Program State");
  auto &IR = Target->getIR();

  if (isa<Constant>(&IR)) {
    if (auto *GV = dyn_cast_or_null<GlobalVariable>(&IR)) {
      if (!GV->hasAtLeastLocalUnnamedAddr())
        return std::make_pair(&IR, -1);
    }
    return std::make_pair(Location, -1);
  }
  return std::make_pair(&IR, -1);
}

bool RDNode::isVisit(Instruction &I) const {

  auto *F = I.getFunction();
  assert(F && "Unexpected Program State");

  auto Iter = Visit.find(F);
  if (Iter == Visit.end())
    return false;

  return Iter->second.find(&I) != Iter->second.end();
}

bool RDNode::isRootDefinition() const {

  assert(Target && "Unexpected Program State");
  return !Target->isAssignable() || ForcedDef;
}

bool RDNode::isMemory() const {

  assert(Target && "Unexpected Program State");
  return isa<RDMemory>(Target.get());
}

bool RDNode::isTimeout() const { return Timeout; }

RDNode &RDNode::operator=(const RDNode &RHS) {

  Idx = RHS.Idx;
  Target = RHS.Target;
  Location = RHS.Location;
  Visit = RHS.Visit;
  FirstUses = RHS.FirstUses;
  ForcedDef = RHS.ForcedDef;
  Timeout = RHS.Timeout;

  return *this;
}

bool RDNode::operator<(const RDNode &Src) const {

  assert((Target && Location && Src.Target && Src.Location) &&
         "Unexpected Program State");
  if (*Target != *Src.Target)
    return *Target < *Src.Target;
  if (Location != Src.Location)
    return Location < Src.Location;
  return Idx < Src.Idx;
}

bool RDNode::operator==(const RDNode &Src) const {

  assert((Target && Location && Src.Target && Src.Location) &&
         "Unexpected Program State");
  return *Target == *Src.Target && Location == Src.Location;
}

raw_ostream &operator<<(raw_ostream &O, const RDNode &Src) {

  assert(Src.Target && Src.Location && "Unexpected Program State");

  auto &Target = Src.getTarget();

  O << "|-[RDNode]---------------\n";
  O << "| Target: "
    << "(" << Src.Idx << ") ";
  Target.dump();
  O << "| Location: " << *Src.Location << "("
    << Src.Location->getFunction()->getName() << ")\n";
  O << "| FirstUses:\n";

  int Cnt = 1;
  for (const auto &FirstUse : Src.FirstUses) {
    O << "|\t[" << Cnt++ << "] " << FirstUse << "\n";
  }

  O << "|-----------------------\n";
  return O;
}

int RDNode::getIdx(Value &V, Instruction &I) const {

  auto T = RDTarget::create(V);
  return getIdx(*T, I);
}

int RDNode::getIdx(const RDTarget &T, Instruction &I) const {

  unsigned MaxIdx = 0;
  if (auto *CB = dyn_cast<llvm::CallBase>(&I))
    MaxIdx = CB->getNumArgOperands();
  else
    MaxIdx = I.getNumOperands();

  for (unsigned S = 0, E = MaxIdx; S < E; ++S) {
    auto OpTarget = RDTarget::create(*I.getOperand(S));

    if (T != *OpTarget)
      continue;

    return S;
  }

  return -1;
}

} // end namespace ftg
