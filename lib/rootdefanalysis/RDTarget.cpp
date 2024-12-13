#include "RDTarget.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include <stack>

using namespace llvm;

static bool isGEPWithNoNotionalOverIndexing(const ConstantExpr* CE) {
  if (CE->getOpcode() != Instruction::GetElementPtr) return false;

  gep_type_iterator GEPI = gep_type_begin(CE), E = gep_type_end(CE);
  User::const_op_iterator OI = std::next(CE->op_begin());

  // Skip the first index, as it has no static limit.
  ++GEPI;
  ++OI;

  // The remaining indices must be compile-time known integers within the
  // bounds of the corresponding notional static array types.
  for (; GEPI != E; ++GEPI, ++OI) {
    ConstantInt *CI = dyn_cast<ConstantInt>(*OI);
    if (!CI) return false;
    if (ArrayType *ATy = dyn_cast<ArrayType>(GEPI.getIndexedType()))
      if (CI->getValue().getActiveBits() > 64 ||
          CI->getZExtValue() >= ATy->getNumElements())
        return false;
  }
  // All the indices checked out.
  return true;
}

namespace ftg {

namespace {

class FindTarget {

public:
  std::set<std::pair<Value *, std::vector<unsigned>>>
  find(Value &Target, RDSpace *Space = nullptr) {

    std::set<std::pair<Value *, std::vector<unsigned>>> Result;

    init();
    push(Target);

    while (auto CurNode = pop()) {
      if (auto *LI = dyn_cast<LoadInst>(CurNode->Target)) {
        push(*LI->getPointerOperand(), CurNode->Indices);
        if (!Space)
          continue;

        for (auto *Alias : Space->getAliases(*LI)) {
          if (Space->dominates(*LI, *Alias))
            continue;

          if (auto *SI = dyn_cast<StoreInst>(Alias)) {
            push(*SI->getValueOperand(), CurNode->Indices);
            continue;
          }

          push(*Alias, CurNode->Indices);
        }
        continue;
      }

      if (auto *CI = dyn_cast<CastInst>(CurNode->Target)) {
        push(*CI->getOperand(0), CurNode->Indices);
        continue;
      }

      if (auto *GEPI = dyn_cast<GetElementPtrInst>(CurNode->Target)) {
        auto Indices = memoryIndices(*GEPI);
        CurNode->Indices.insert(CurNode->Indices.begin(), Indices.begin(),
                                Indices.end());
        push(*GEPI->getOperand(0), CurNode->Indices);
        continue;
      }

      if (auto *CE = dyn_cast<ConstantExpr>(CurNode->Target)) {
        if (CE->isCast()) {
          push(*CE->getOperand(0), CurNode->Indices);
          continue;
        } else if (isGEPWithNoNotionalOverIndexing(CE)) {
          auto Indices = memoryIndices(*CE);
          CurNode->Indices.insert(CurNode->Indices.begin(), Indices.begin(),
                                  Indices.end());
          push(*CE->getOperand(0), CurNode->Indices);
          continue;
        }
      }

      Result.emplace(CurNode->Target, CurNode->Indices);
    }

    return Result;
  }

private:
  typedef struct _Node {

    _Node(Value &Target, std::vector<unsigned> Indices)
        : Target(&Target), Indices(Indices) {}

    llvm::Value *Target;
    std::vector<unsigned> Indices;

  } Node;

  void init() {

    Visit.clear();
    while (Stack.size() > 0)
      Stack.pop();
  }

  void push(llvm::Value &Target, std::vector<unsigned> Indices = {}) {
    if (Visit.find(&Target) != Visit.end())
      return;

    Visit.insert(&Target);
    Stack.push(std::make_shared<Node>(Target, Indices));
  }

  std::shared_ptr<Node> pop() {

    if (Stack.size() == 0)
      return nullptr;

    auto Result = Stack.top();
    Stack.pop();

    return Result;
  }

  std::vector<unsigned> memoryIndices(llvm::Value &V) const {

    std::vector<unsigned> MemoryIndices;
    std::vector<llvm::Value *> GEPIndices;

    if (auto *GEPI = dyn_cast<GetElementPtrInst>(&V)) {
      for (auto &Index : GEPI->indices()) {
        GEPIndices.push_back(Index.get());
      }
    }

    if (auto *CE = dyn_cast<ConstantExpr>(&V)) {
      for (unsigned S = 1, E = CE->getNumOperands(); S < E; ++S) {
        GEPIndices.push_back(CE->getOperand(S));
      }
    }

    if (GEPIndices.size() > 1) {
      auto *Index = GEPIndices[0];
      assert(Index && "Unexpected Program State");

      if (!isa<ConstantInt>(Index)) {
        llvm::outs() << "Unexpected Memory Access Instruction: " << V << "\n";
        return MemoryIndices;
      }

      auto &CI = *dyn_cast<ConstantInt>(Index);
      auto RealValue = CI.getZExtValue();
      if (RealValue != 0) {
        llvm::outs() << "Unexpected Memory Access Instruction: " << V << "\n";
        return MemoryIndices;
      }

      GEPIndices.erase(GEPIndices.begin());
    }

    for (unsigned S = 0, E = GEPIndices.size(); S < E; ++S) {
      auto *Index = GEPIndices[S];
      assert(Index && "Unexpected Program State");

      if (!isa<ConstantInt>(Index)) {
        GEPIndices.clear();
        return MemoryIndices;
      }

      auto &CI = *dyn_cast<ConstantInt>(Index);
      auto RealValue = CI.getZExtValue();
      MemoryIndices.push_back(RealValue);
    }

    return MemoryIndices;
  }

  std::stack<std::shared_ptr<Node>> Stack;
  std::set<llvm::Value *> Visit;
};

} // namespace

RDTarget::RDTarget(MemoryType Kind, llvm::Value &IR,
                   std::vector<unsigned> MemoryIndices)
    : Kind(Kind), IR(&IR), MemoryIndices(MemoryIndices) {}

RDTarget::RDTarget(const RDTarget &Rhs) { *this = Rhs; }

std::shared_ptr<RDTarget> RDTarget::create(llvm::Value &IR) {

  auto Cands = FindTarget().find(IR);
  assert(Cands.size() == 1);
  auto Cand = *Cands.begin();
  assert(Cand.first && "Unexpected Program State");

  return create(*Cand.first, Cand.second);
}

std::shared_ptr<RDTarget>
RDTarget::create(llvm::Value &IR, std::vector<unsigned> MemoryIndices) {

  if (isa<ConstantData>(&IR) || isa<ConstantExpr>(&IR) ||
      isa<ConstantAggregate>(&IR) || isa<Function>(&IR)) {

    return std::make_shared<RDConstant>(IR, MemoryIndices);
  }

  if (auto *GV = dyn_cast_or_null<GlobalValue>(&IR)) {
    if (GV->hasGlobalUnnamedAddr()) {
      return std::make_shared<RDConstant>(IR, MemoryIndices);
    }
  }

  if (isa<AllocaInst>(&IR) || isa<GlobalVariable>(&IR) || isa<Argument>(&IR)) {

    return std::make_shared<RDMemory>(IR, MemoryIndices);
  }

  return std::make_shared<RDRegister>(IR);
}

std::shared_ptr<RDTarget> RDTarget::create(const RDTarget &Rhs) {

  if (isa<RDConstant>(Rhs)) {

    return std::make_shared<RDConstant>(*Rhs.IR, Rhs.MemoryIndices);
  }

  if (isa<RDMemory>(Rhs)) {

    return std::make_shared<RDMemory>(*Rhs.IR, Rhs.MemoryIndices);
  }

  return std::make_shared<RDRegister>(*Rhs.IR);
}

std::set<std::shared_ptr<RDTarget>> RDTarget::create(Value &IR,
                                                     RDSpace *Space) {

  std::set<std::shared_ptr<RDTarget>> Targets;

  for (auto &Cand : FindTarget().find(IR, Space)) {
    assert(Cand.first && "Unexpected Program State");

    auto Target = create(*Cand.first, Cand.second);
    if (std::find_if(Targets.begin(), Targets.end(),
                     [&Target](const auto &Src) { return *Target == *Src; }) ==
        Targets.end())
      Targets.insert(Target);
  }

  return Targets;
}

RDTarget::MemoryType RDTarget::getKind() const { return Kind; }

llvm::Value &RDTarget::getIR() const {

  assert(IR && "Unexpected Program State");
  return *IR;
}

const std::vector<unsigned> &RDTarget::getMemoryIndices() const {

  return MemoryIndices;
}

bool RDTarget::isGlobalVariable() const {

  if (!isAssignable())
    return false;
  return isa<GlobalVariable>(IR);
}

bool RDTarget::includes(const RDTarget &Target) const {

  if (IR != &Target.getIR())
    return false;

  auto TargetMemoryIndices = Target.getMemoryIndices();
  if (MemoryIndices.size() > TargetMemoryIndices.size())
    return false;

  for (unsigned S = 0, E = MemoryIndices.size(); S < E; ++S) {
    if (MemoryIndices[S] != TargetMemoryIndices[S])
      return false;
  }

  return true;
}

bool RDTarget::operator==(const RDTarget &Rhs) const {

  assert(IR && "Unexpected Program State");

  auto &RhsIR = Rhs.getIR();

  if (IR != &RhsIR)
    return false;

  if (isa<RDMemory>(this) && isa<RDMemory>(&Rhs)) {
    auto &ThisMem = *dyn_cast<RDMemory>(this);
    auto &RhsMem = *dyn_cast<RDMemory>(&Rhs);

    auto &ThisMemIndices = ThisMem.getMemoryIndices();
    auto &RhsMemIndices = RhsMem.getMemoryIndices();

    auto ThisMemIndicesSize = ThisMemIndices.size();
    auto RhsMemIndicesSize = RhsMemIndices.size();

    if (ThisMemIndicesSize != RhsMemIndicesSize)
      return false;

    for (unsigned S = 0; S < ThisMemIndicesSize; ++S) {
      if (ThisMemIndices[S] != RhsMemIndices[S])
        return false;
    }
  }

  return true;
}

bool RDTarget::operator!=(const RDTarget &Rhs) const { return !(*this == Rhs); }

bool RDTarget::operator<(const RDTarget &Rhs) const {

  assert(IR && "Unexpected Program State");

  auto &RhsIR = Rhs.getIR();
  auto IRResult = IR < &RhsIR;

  if (IR != &RhsIR)
    return IRResult;

  if (isa<RDMemory>(this) && isa<RDMemory>(&Rhs)) {
    auto &ThisMem = *dyn_cast<RDMemory>(this);
    auto &RhsMem = *dyn_cast<RDMemory>(&Rhs);

    auto &ThisMemIndices = ThisMem.getMemoryIndices();
    auto &RhsMemIndices = RhsMem.getMemoryIndices();

    auto ThisMemIndicesSize = ThisMemIndices.size();
    auto RhsMemIndicesSize = RhsMemIndices.size();

    if (ThisMemIndicesSize != RhsMemIndicesSize) {
      return ThisMemIndicesSize < RhsMemIndicesSize;
    }

    for (unsigned S = 0; S < ThisMemIndicesSize; ++S) {
      if (ThisMemIndices[S] != RhsMemIndices[S]) {
        return ThisMemIndices[S] < RhsMemIndices[S];
      }
    }

    return ThisMemIndices[ThisMemIndicesSize - 1] <
           RhsMemIndices[ThisMemIndicesSize - 1];
  }

  return IRResult;
}

RDTarget &RDTarget::operator=(const RDTarget &Rhs) {

  Kind = Rhs.Kind;
  IR = Rhs.IR;
  MemoryIndices = Rhs.MemoryIndices;
  return *this;
}

void RDTarget::setMemoryIndices(std::vector<unsigned> MemoryIndices) {

  this->MemoryIndices = MemoryIndices;
}

bool RDRegister::classof(const RDTarget *Target) {

  if (!Target)
    return false;
  return Target->getKind() == RDTarget::MemoryType_REG;
}

RDRegister::RDRegister(llvm::Value &IR) : RDTarget(MemoryType_REG, IR) {}

bool RDRegister::isAssignable() const {

  assert(IR && "Unexpected Program State");
  if (auto *CB = dyn_cast<CallBase>(IR)) {
    auto *F = CB->getCalledFunction();
    if (!F)
      return false;
    if (F->isDeclaration() && F->arg_size() == 0)
      return false;
  }

  return true;
}

void RDRegister::dump() const { llvm::outs() << "[Register] " << *IR << "\n"; }

bool RDConstant::classof(const RDTarget *Target) {

  if (!Target)
    return false;
  return Target->getKind() == RDTarget::MemoryType_CONST;
}

RDConstant::RDConstant(llvm::Value &IR, std::vector<unsigned> MemoryIndices)
    : RDTarget(MemoryType_CONST, IR, MemoryIndices) {}

bool RDConstant::isAssignable() const { return false; }

void RDConstant::dump() const {

  llvm::outs() << "[Constant] " << *IR << "\n";
  for (auto &MemoryIndex : MemoryIndices) {
    llvm::outs() << "[" << MemoryIndex << "]";
  }
  llvm::outs() << "\n";
}

bool RDMemory::classof(const RDTarget *Target) {

  if (!Target)
    return false;
  return Target->getKind() == RDTarget::MemoryType_MEM;
}

RDMemory::RDMemory(llvm::Value &Target, std::vector<unsigned> MemoryIndices)
    : RDTarget(MemoryType_MEM, Target, MemoryIndices) {}

bool RDMemory::isAssignable() const {

  assert(IR && "Unexpected Program State");
  if (auto *GV = dyn_cast<GlobalVariable>(IR)) {
    if (GV->isConstant())
      return false;
  }

  return true;
}

void RDMemory::dump() const {

  llvm::outs() << "[Memory] " << *IR;
  for (auto &MemoryIndex : MemoryIndices) {
    llvm::outs() << "[" << MemoryIndex << "]";
  }
  llvm::outs() << "\n";
}

} // end namespace ftg
