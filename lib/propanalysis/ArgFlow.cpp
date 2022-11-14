#include "ftg/propanalysis/ArgFlow.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include <list>

using namespace llvm;

namespace ftg {

namespace {

/// ref: http://llvm.org/doxygen/AsmWriter_8cpp_source.html#l00446
/// Write the specified type to the specified raw_ostream, making use of type
/// names or up references to shorten the type name where possible.
llvm::StringRef getTypeStr(llvm::Type *Ty) {
  if (!Ty)
    return "";

  switch (Ty->getTypeID()) {
  case llvm::Type::VoidTyID:
    return "void";
  case llvm::Type::HalfTyID:
    return "half";
  case llvm::Type::FloatTyID:
    return "float";
  case llvm::Type::DoubleTyID:
    return "double";
  case llvm::Type::X86_FP80TyID:
    return "x86_fp80";
  case llvm::Type::FP128TyID:
    return "fp128";
  case llvm::Type::PPC_FP128TyID:
    return "ppc_fp128";
  case llvm::Type::LabelTyID:
    return "label";
  case llvm::Type::MetadataTyID:
    return "metadata";
  case llvm::Type::X86_MMXTyID:
    return "x86_mmx";
  case llvm::Type::TokenTyID:
    return "token";
  case llvm::Type::IntegerTyID:
    return "int";
  case llvm::Type::FunctionTyID:
    return "function";
  case llvm::Type::StructTyID: {
    llvm::StructType *STy = llvm::cast<llvm::StructType>(Ty);
    if (!STy->getName().empty()) {
      return STy->getName();
    }
    return "struct";
  }
  case llvm::Type::ArrayTyID:
    return "array";
#if LLVM_VERSION_MAJOR >= 12
  case llvm::Type::FixedVectorTyID:
  case llvm::Type::ScalableVectorTyID:
#else
  case llvm::Type::VectorTyID:
#endif
  {
    return "vector";
  }
  case llvm::Type::PointerTyID: {
    llvm::PointerType *PTy = llvm::cast<llvm::PointerType>(Ty);
    return getTypeStr(PTy->getElementType());
  }
  default:
    return "";
  }
}

} // namespace

bool isUnionFieldAccess(const llvm::Instruction *I) {
  if (I->getNumOperands() == 0)
    return false;

  auto *Op0 = I->getOperand(0);
  if (!Op0)
    return false;

  return isa<BitCastInst>(I) &&
         getTypeStr(Op0->getType()).split('.').first == "union";
}

ArgFlow::ArgFlow(Argument &A) : Arg(A) {}

void ArgFlow::setState(AnalysisState State) { this->State = State; }

void ArgFlow::setFilePathString() { IsFilePathString = true; }

void ArgFlow::setStruct(StructType *ST) {
  if (STInfo)
    return;

  STInfo = std::make_shared<StructInfo>(ST);

  // If Struct is a field in struct(nested struct),
  // the depth(from Argument) is increased
  if (FDInfo)
    Depth++;
}

void ArgFlow::setIsVariableLenArray(bool Value) { IsVariableLenArray = Value; }

void ArgFlow::setIsArray(bool Value) { IsArray = Value; }

void ArgFlow::setSizeArg(Argument &A) {
  SizeArgs.clear();
  SizeArgs.emplace(&A);
}

void ArgFlow::setToArrLen(Argument &RelatedArg) {
  IsArrayLen = true;
  ArrayArgs.insert(&RelatedArg);
}

void ArgFlow::setToArrLen(unsigned FieldNum) {
  if (!FDInfo)
    return;

  IsArrayLen = true;
  FDInfo->ArrayFields.insert(FieldNum);
}

bool ArgFlow::isAllocSize() const {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  return IsAllocSize;
}

bool ArgFlow::isArray() const {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  return IsArray;
}

bool ArgFlow::isArrayLen() const {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  return IsArrayLen;
}

bool ArgFlow::hasLenRelatedArg() {
  return (!SizeArgs.empty() || !ArrayArgs.empty());
}

bool ArgFlow::isFilePathString() { return IsFilePathString; }

bool ArgFlow::isStruct() {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  if (STInfo)
    return true;
  return false;
}

bool ArgFlow::isField() {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  if (FDInfo)
    return true;
  return false;
}

bool ArgFlow::isUsedByRet() {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  return IsUsedByRet;
}

bool ArgFlow::isVariableLenArray() const { return IsVariableLenArray; }

Argument &ArgFlow::getLLVMArg() { return Arg; }

AnalysisState ArgFlow::getState() const { return this->State; }

ArgDir ArgFlow::getArgDir() const {
  assert(State != AnalysisState_Not_Analyzed &&
         "An argument has not been analyzed yet");
  return Direction;
}

// TODO: implement to handle more than one related args
unsigned ArgFlow::getLenRelatedArgNo() {
  assert(hasLenRelatedArg() &&
         "An ArgFlow can't get length related arg number!");
  return !SizeArgs.empty() ? (*SizeArgs.begin())->getArgNo()
                           : (*ArrayArgs.begin())->getArgNo();
}

std::shared_ptr<ArgFlow::StructInfo> ArgFlow::getStructInfo() { return STInfo; }

std::shared_ptr<ArgFlow::FieldInfo> ArgFlow::getFieldInfo() { return FDInfo; }

ArgFlow &ArgFlow::getOrCreateFieldFlow(unsigned FieldNum) {
  if (STInfo->FieldResults[FieldNum]) {
    return *STInfo->FieldResults[FieldNum];
  }

  std::shared_ptr<ArgFlow> FieldResult = std::make_shared<ArgFlow>(Arg);
  FieldResult->setField(FieldNum, *this); // assign fieldinfo
  FieldResult->Depth = Depth;
  FieldResult->setState(AnalysisState_Analyzing);
  STInfo->FieldResults[FieldNum] = FieldResult;
  return *FieldResult;
}

std::set<size_t> &ArgFlow::getArrIndices() { return ArrIndexes; }

const std::set<Argument *> &ArgFlow::getSizeArgs() { return SizeArgs; }

void ArgFlow::mergeAllocSize(const ArgFlow &CalleeArgFlowResult) {
  this->IsAllocSize |= CalleeArgFlowResult.IsAllocSize;

  // if argument/value is struct, merge each fields result also
  if (!CalleeArgFlowResult.STInfo)
    return;

  if (!isStruct())
    setStruct(CalleeArgFlowResult.STInfo->StructType);

  for (auto Result : CalleeArgFlowResult.STInfo->FieldResults) {
    if (!Result.second)
      continue;
    unsigned FieldNum = Result.first;
    ArgFlow &FieldResult = this->getOrCreateFieldFlow(FieldNum);
    ArgFlow &CalleeResult = *Result.second;
    FieldResult.mergeAllocSize(CalleeResult);
    this->STInfo->FieldsAllocSize |= FieldResult.IsAllocSize;
  }
}

void ArgFlow::mergeDirection(const ArgFlow &CalleeArgFlowResult) {
  this->Direction |= CalleeArgFlowResult.Direction;

  // if argument/value is struct, merge each fields result also
  if (!CalleeArgFlowResult.STInfo)
    return;

  if (!isStruct())
    setStruct(CalleeArgFlowResult.STInfo->StructType);

  for (auto Result : CalleeArgFlowResult.STInfo->FieldResults) {
    if (!Result.second)
      continue;
    unsigned FieldNum = Result.first;
    ArgFlow &FieldResult = this->getOrCreateFieldFlow(FieldNum);
    ArgFlow &CalleeResult = *Result.second;
    FieldResult.mergeDirection(CalleeResult);
    STInfo->FieldsDirection |= FieldResult.Direction;
  }
}

void ArgFlow::mergeArray(const ArgFlow &CalleeArgFlowResult, CallBase &C) {

  IsArray |= CalleeArgFlowResult.IsArray;
  IsVariableLenArray |= CalleeArgFlowResult.IsVariableLenArray;
  IsFilePathString |= CalleeArgFlowResult.IsFilePathString;

  auto CalleeArrIdxes = CalleeArgFlowResult.ArrIndexes;
  ArrIndexes.insert(CalleeArrIdxes.begin(), CalleeArrIdxes.end());

  // If the value is Array and this value is a field in struct,
  // merge field info also.
  if (CalleeArgFlowResult.FDInfo) {
    if (CalleeArgFlowResult.IsArray)
      this->FDInfo->SizeFields.insert(
          CalleeArgFlowResult.FDInfo->SizeFields.begin(),
          CalleeArgFlowResult.FDInfo->SizeFields.end());
    else if (CalleeArgFlowResult.IsArrayLen)
      this->FDInfo->ArrayFields.insert(
          CalleeArgFlowResult.FDInfo->ArrayFields.begin(),
          CalleeArgFlowResult.FDInfo->ArrayFields.end());
  } else if (CalleeArgFlowResult.IsArray) {
    // Result is for Argument, need to merge Size/Array Argument Info
    // Find arguments of caller function (==current function) that
    // a size argument in the callee function depends on.
    for (auto *SizeArg : CalleeArgFlowResult.SizeArgs) {
      auto *V = C.getArgOperand(SizeArg->getArgNo());
      if (this->FDInfo) {
        collectRelatedLengthField(*V);
        ArgFlow &ParentResult = this->FDInfo->Parent;
        for (unsigned FieldNum : this->FDInfo->SizeFields) {
          ArgFlow &LenFieldResult = ParentResult.getOrCreateFieldFlow(FieldNum);
          LenFieldResult.setToArrLen(this->FDInfo->FieldNum);
        }
      } else
        collectRelatedLengthArg(*V);
    }
  }

  // if argument/value is struct, merge each fields result also
  if (!CalleeArgFlowResult.STInfo)
    return;

  if (!this->isStruct())
    this->setStruct(CalleeArgFlowResult.STInfo->StructType);

  for (auto Result : CalleeArgFlowResult.STInfo->FieldResults) {
    if (!Result.second)
      continue;
    unsigned FieldNum = Result.first;
    ArgFlow &FieldResult = this->getOrCreateFieldFlow(FieldNum);
    ArgFlow &CalleeResult = *Result.second;
    FieldResult.mergeArray(CalleeResult, C);
  }
}

// Used for analyze ArrayLen
// First tracking the value used as array index (findRelatedField) then
// if the value is related to Argument/Struct Field,
// collect the value(collectRelatedLengthField)
// The value will has the attriube as ArrayLen
void ArgFlow::collectRelatedLengthField(Value &V) {
  SmallSet<Value *, 8> Visited;
  int FieldNum = findRelatedField(V, Visited, FDInfo->Parent);
  if (FieldNum >= 0)
    FDInfo->SizeFields.insert(FieldNum);
}

/*
 * Trace backward using use-def chains until we reach argument definition(s).
 */
void ArgFlow::collectRelatedLengthArg(Value &V) {
  std::set<llvm::Value *> Visit;
  if (auto *RelatedArg = findRelatedArg(V, Visit))
    this->SizeArgs.insert(RelatedArg);
}

ArgFlow &ArgFlow::operator|=(const ArgDir &ArgDir) {
  Direction |= ArgDir;
  return *this;
}

void ArgFlow::setArgDir(unsigned ArgDir) { Direction = ArgDir; }

Argument *ArgFlow::findRelatedArg(Value &V, std::set<Value *> &Visit) {
  if (Visit.find(&V) != Visit.end() || isa<Constant>(&V) || isa<CmpInst>(&V) ||
      isa<GetElementPtrInst>(&V)) {
    return nullptr;
  } else if (auto *I = dyn_cast<Instruction>(&V)) {
    if (I->isTerminator()) {
      return nullptr;
    }
  }
  Visit.insert(&V);

  if (auto *A = dyn_cast<Argument>(&V))
    return A;

  // traverse Operands and Users
  if (auto *I = dyn_cast<Instruction>(&V)) {
    if (isUnionFieldAccess(I) || isa<CallBase>(I))
      return nullptr;

    for (auto *O : I->operand_values()) {
      if (isa<Constant>(O))
        continue;

      if (auto *RelatedArg = findRelatedArg(*O, Visit))
        return RelatedArg;
    }
  }

  // FIXME: it is workaround to reverse order of users
  std::list<User *> users;
  for (auto *U : V.users())
    users.push_front(U);

  for (auto *U : users) {
    if (auto *RelatedArg = findRelatedArg(*U, Visit))
      return RelatedArg;
  }

  return nullptr;
}

void ArgFlow::setAllocSize() { IsAllocSize = true; }

void ArgFlow::setUsedByRet(bool ArgRet) { IsUsedByRet = ArgRet; }

void ArgFlow::setField(unsigned FieldNum, ArgFlow &Parent) {
  // assign FieldInfo
  FDInfo = std::make_shared<FieldInfo>(FieldNum, Parent);
}

int ArgFlow::findRelatedField(Value &V, SmallSet<Value *, 8> &VisitedValues,
                              ArgFlow &ParentResult) {
  int RelatedField = -1;

  if (VisitedValues.find(&V) != VisitedValues.end() || isa<Constant>(&V) ||
      isa<CmpInst>(&V)) {
    return RelatedField;
  }

  if (auto *I = dyn_cast<Instruction>(&V)) {
    if (I->isTerminator())
      return RelatedField;
    VisitedValues.insert(&V);

    if (isUnionFieldAccess(I) || isa<CallBase>(I)) {
      return RelatedField;
    }

    if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
      for (auto Result : ParentResult.STInfo->FieldResults) {
        if (!Result.second)
          continue;
        unsigned FieldNum = Result.first;
        auto FieldResult = *Result.second;

        if (FieldResult.FDInfo->Values.find(GEP) !=
            FieldResult.FDInfo->Values.end()) {
          return FieldNum;
        }
      }
      return RelatedField;
    }

    for (auto *O : I->operand_values()) {
      if (isa<Constant>(O))
        continue;

      RelatedField = this->findRelatedField(*O, VisitedValues, ParentResult);
      if (RelatedField > -1)
        return RelatedField;
    }
  }

  for (auto *U : V.users()) {
    RelatedField = findRelatedField(*U, VisitedValues, ParentResult);
    if (RelatedField > -1)
      return RelatedField;
  }

  return RelatedField;
}

ArgFlow::FieldInfo::FieldInfo(unsigned FieldNum, ArgFlow &Parent)
    : FieldNum(FieldNum), Parent(Parent) {}

bool ArgFlow::FieldInfo::hasLenRelatedField() {
  return (!SizeFields.empty() || !ArrayFields.empty());
}

ArgFlow::StructInfo::StructInfo(llvm::StructType *ST) : StructType(ST) {}

} // namespace ftg
