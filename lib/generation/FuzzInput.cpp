#include "ftg/generation/FuzzInput.h"
#include "ftg/utils/AssignUtil.h"
#include "ftg/utils/StringUtil.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace ftg {

using DefPtr = std::shared_ptr<const Definition>;
using InputPtr = std::shared_ptr<FuzzInput>;
using NoInputPtr = std::shared_ptr<FuzzNoInput>;

FuzzInput::FuzzInput(std::shared_ptr<const Definition> Def)
    : ArrayDef(nullptr), ArrayLenDef(nullptr), Def(Def) {
  FuzzVarName = util::getAvasFuzzVarName(Def->ID);
  ProtoVarName = util::getProtoVarName(Def->ID);
  LocalVar.setName(ProtoVarName);
  LocalVar.setType(getFTGType());
  LocalPtrVar = LocalVar;
}

const FuzzInput *FuzzInput::getCopyFrom() const { return CopyFrom.get(); }

unsigned FuzzInput::getID() const { return getDef().ID; }

const Definition &FuzzInput::getDef() const {

  assert(Def && "Unexpected Program State");
  return *Def;
}

std::shared_ptr<const Definition> &FuzzInput::getDefPtr() { return Def; }

Type &FuzzInput::getFTGType() {
  assert(getDef().DataType && "Unexpected Program State");
  return *getDef().DataType;
}

AssignVar &FuzzInput::getLocalVar() { return LocalVar; }

AssignVar &FuzzInput::getLocalPtrVar() { return LocalPtrVar; }

std::string FuzzInput::getFuzzVarName() const { return FuzzVarName; }

std::string FuzzInput::getProtoVarName() const { return ProtoVarName; }

void FuzzInput::setCopyFrom(std::shared_ptr<FuzzInput> CopyFrom) {
  this->CopyFrom = CopyFrom;
}

std::string FuzzNoInput::getExtraInfo() const { return ""; }

FuzzNoInput::FuzzNoInput(Type Kind, std::shared_ptr<const Definition> Def)
    : Kind(Kind), Def(Def) {}

unsigned FuzzNoInput::getDefID() const { return getDef().ID; }

const Definition &FuzzNoInput::getDef() const {

  assert(Def && "Unexpected Program State");
  return *Def;
}

FuzzNoInput::Type FuzzNoInput::getType() const { return Kind; }

raw_ostream &operator<<(raw_ostream &O, const FuzzNoInput &RHS) {

  auto &Info = RHS.getDef();
  O << Info.ID << " : " << RHS.getAsString(RHS.Kind);

  std::string Detail = "Path: " + Info.Path + "(" +
                       std::to_string(Info.Offset) + "/" +
                       std::to_string(Info.Length) + ")";

  auto ExtraInfo = RHS.getExtraInfo();
  if (!ExtraInfo.empty()) {
    Detail += ", " + ExtraInfo;
  }

  O << " (" << Detail << ")\n";
  return O;
}

std::string FuzzNoInput::getAsString(Type Kind) const {

  const std::map<Type, std::string> TypeToStrMap = {
      {FuzzNoInput::Type_MeshArrayRelation, "Mesh Array Relation"},
      {FuzzNoInput::Type_GroupUnsupport, "Group Unsupport"},
      {FuzzNoInput::Type_NoNamedArray, "No Named Array"},
      {FuzzNoInput::Type_Unknown, "Unknown"},
  };

  auto Iter = TypeToStrMap.find(Kind);
  assert(Iter != TypeToStrMap.end() && "Unexpected Program State");

  return Iter->second;
}

std::string NoInputByGroup::getExtraInfo() const {

  return "By: " + std::to_string(DefIDAffectedBy);
}

NoInputByGroup::NoInputByGroup(std::shared_ptr<const Definition> Def,
                               unsigned DefIDAffectedBy)
    : FuzzNoInput(Type_GroupUnsupport, Def), DefIDAffectedBy(DefIDAffectedBy) {}

std::pair<std::map<unsigned, std::shared_ptr<FuzzInput>>,
          std::map<unsigned, std::shared_ptr<FuzzNoInput>>>
FuzzInputGenerator::generate(const std::vector<DefPtr> &Defs) {

  auto DefMap = generateDefMap(Defs);
  std::tie(FuzzInputMap, FuzzNoInputMap) = generateFuzzInputMap(DefMap);
  applyArrayProperty(FuzzInputMap, FuzzNoInputMap, DefMap);
  return std::make_pair(FuzzInputMap, FuzzNoInputMap);
}

bool FuzzInputGenerator::isMultiDimensionArray(const Type &T) {

  if (!T.isArrayPtr())
    return false;

  const auto *PT = T.getPointeeType();
  if (!PT || &T == PT)
    return false;

  return PT->isArrayPtr() || PT->isFixedLengthArrayPtr();
}

std::map<unsigned, std::shared_ptr<const Definition>>
FuzzInputGenerator::generateDefMap(
    const std::vector<std::shared_ptr<const Definition>> &Defs) const {

  std::map<unsigned, std::shared_ptr<const Definition>> Result;

  for (auto Def : Defs) {
    assert(Def && "Unexpected Program State");
    Result.emplace(Def->ID, Def);
  }

  return Result;
}

void FuzzInputGenerator::updateGroups(std::vector<std::set<unsigned>> &Result,
                                      const std::set<unsigned> &Group) const {

  for (auto &ResultGroup : Result) {
    for (auto E : Group) {
      if (ResultGroup.find(E) == ResultGroup.end())
        continue;

      ResultGroup.insert(Group.begin(), Group.end());
      return;
    }
  }

  Result.push_back(Group);
}

std::pair<std::map<unsigned, std::shared_ptr<FuzzInput>>,
          std::map<unsigned, std::shared_ptr<FuzzNoInput>>>
FuzzInputGenerator::generateFuzzInputMap(
    const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap) const {

  std::map<unsigned, std::shared_ptr<FuzzInput>> FuzzInputMap;
  std::map<unsigned, std::shared_ptr<FuzzNoInput>> FuzzNoInputMap;

  for (auto Iter : DefMap) {
    auto Def = Iter.second;
    assert(Def && "Unexpected Program State");

    auto NoInput = FuzzNoInputFactory().generate(Def);
    if (NoInput) {
      FuzzNoInputMap.emplace(NoInput->getDefID(), NoInput);
      continue;
    }

    auto Input = FuzzInputFactory().generate(Def);
    assert(Input && "Unexpected Program State");
    FuzzInputMap.emplace(Input->getDef().ID, Input);
  }

  return std::make_pair(FuzzInputMap, FuzzNoInputMap);
}

void FuzzInputGenerator::applyArrayProperty(
    std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
    std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
    const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap) const {

  auto ArrayGroups = generateArrayGroups(DefMap);
  updateFuzzInputMapAndArrayGroups(FuzzInputMap, FuzzNoInputMap, ArrayGroups);
  applyArrayPolicies(FuzzInputMap, FuzzNoInputMap, ArrayGroups);
  updateArrayInfo(FuzzInputMap, ArrayGroups);
}

std::vector<std::set<unsigned>> FuzzInputGenerator::generateArrayGroups(
    const std::map<unsigned, std::shared_ptr<const Definition>> &DefMap) const {

  std::vector<std::set<unsigned>> Result;

  for (auto Iter : DefMap) {
    auto &Def = Iter.second;
    assert(Def && "Unexpected Program State");

    std::set<unsigned> Group = {Def->ID};
    if (Def->Array) {
      for (auto ArrayLenDefID : Def->ArrayLenIDs) {
        if (DefMap.find(ArrayLenDefID) == DefMap.end())
          continue;
        Group.insert(ArrayLenDefID);
      }
    } else if (Def->ArrayLen) {
      for (auto ArrayDefID : Def->ArrayIDs) {
        if (DefMap.find(ArrayDefID) == DefMap.end())
          continue;
        Group.insert(ArrayDefID);
      }
    } else
      continue;

    updateGroups(Result, Group);
  }

  return Result;
}

void FuzzInputGenerator::updateFuzzInputMapAndArrayGroups(
    std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
    std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
    std::vector<std::set<unsigned>> &ArrayGroups) const {

  std::set<std::shared_ptr<FuzzNoInput>> TemporalFuzzNoInputs;

  for (auto NIMIter : FuzzNoInputMap) {
    auto NoInputDefID = NIMIter.first;
    auto GroupIter =
        std::find_if(ArrayGroups.begin(), ArrayGroups.end(),
                     [NoInputDefID](const std::set<unsigned> &Group) {
                       return Group.find(NoInputDefID) != Group.end();
                     });
    if (GroupIter == ArrayGroups.end())
      continue;

    for (auto Member : *GroupIter) {
      auto IMIter = FuzzInputMap.find(Member);
      if (IMIter == FuzzInputMap.end())
        continue;

      assert(IMIter->second && "Unexpected Program State");
      auto NoInput = FuzzNoInputFactory().generate(IMIter->second->getDefPtr(),
                                                   NIMIter.first);
      TemporalFuzzNoInputs.insert(NoInput);
      FuzzInputMap.erase(IMIter);
    }
    ArrayGroups.erase(GroupIter);
  }

  for (auto &NoInput : TemporalFuzzNoInputs) {
    assert(NoInput && "Unexpected Program State");
    auto Insert = FuzzNoInputMap.emplace(NoInput->getDefID(), NoInput);
    assert(Insert.second && "Unexpected Program State");
  }
}

void FuzzInputGenerator::applyArrayPolicies(
    std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
    std::map<unsigned, std::shared_ptr<FuzzNoInput>> &FuzzNoInputMap,
    std::vector<std::set<unsigned>> &ArrayGroups) const {

  for (int S = 0; S < (int)ArrayGroups.size(); ++S) {
    auto &Group = ArrayGroups[S];
    std::set<unsigned> ArrayDefIDs;
    std::set<unsigned> ArrayLenDefIDs;

    for (auto Member : Group) {
      auto Iter = FuzzInputMap.find(Member);
      assert(Iter != FuzzInputMap.end() && "Unexpected Program State");
      assert(Iter->second && "Unexpected Program State");

      auto &Def = Iter->second->getDef();
      if (Def.Array)
        ArrayDefIDs.emplace(Def.ID);
      else if (Def.ArrayLen)
        ArrayLenDefIDs.emplace(Def.ID);
    }

    if (ArrayDefIDs.size() != 1) {
      for (auto Member : Group) {
        auto Iter = FuzzInputMap.find(Member);
        assert(Iter != FuzzInputMap.end() && "Unexpected Program State");
        assert(Iter->second && "Unexpected Program State");

        auto Def = Iter->second->getDefPtr();
        auto NoInput = FuzzNoInputFactory().generateMeshArrayRelation(Def);
        assert(NoInput && "Unexpected Program State");

        FuzzNoInputMap.emplace(NoInput->getDefID(), NoInput);
        FuzzInputMap.erase(Iter);
      }
      ArrayGroups.erase(ArrayGroups.begin() + S);
      S--;
      continue;
    }

    if (ArrayLenDefIDs.size() > 1) {
      std::vector<unsigned> ArrayLenDefIDVec(ArrayLenDefIDs.begin(),
                                             ArrayLenDefIDs.end());
      auto Iter = FuzzInputMap.find(ArrayLenDefIDVec[0]);
      assert(Iter != FuzzInputMap.end() && "Unexpected Program State");
      assert(Iter->second && "Unexpected Program State");

      auto &BaseInput = Iter->second;
      for (unsigned S1 = 1, E1 = ArrayLenDefIDVec.size(); S1 < E1; ++S1) {
        auto Iter = FuzzInputMap.find(ArrayLenDefIDVec[S1]);
        assert(Iter != FuzzInputMap.end() && "Unexpected Program State");
        assert(Iter->second && "Unexpected Program State");

        Iter->second->setCopyFrom(BaseInput);
      }
    }
  }
}

void FuzzInputGenerator::updateArrayInfo(
    std::map<unsigned, std::shared_ptr<FuzzInput>> &FuzzInputMap,
    const std::vector<std::set<unsigned>> &ArrayGroups) const {

  for (auto &ArrayGroup : ArrayGroups) {
    const FuzzInput *ArrayDef = nullptr;
    const FuzzInput *ArrayLenDef = nullptr;

    for (auto &ID : ArrayGroup) {
      auto Iter = FuzzInputMap.find(ID);
      assert(Iter != FuzzInputMap.end() && "Unexpected Program State");

      const auto *Input = Iter->second.get();
      assert(Input && "Unexpected Program State");

      if (Input->getCopyFrom())
        continue;

      const auto &Def = Input->getDef();
      if (Def.Array) {
        if (ArrayDef) {
          assert(ArrayDef == Input && "Unexpected Program State");
        }
        ArrayDef = Input;
      } else if (Def.ArrayLen) {
        if (ArrayLenDef) {
          assert(ArrayLenDef == Input && "Unexpected Program State");
        }
        ArrayLenDef = Input;
      }
    }

    assert(ArrayDef && "Unexpected Program State");
    if (!ArrayLenDef)
      continue;

    const_cast<FuzzInput *>(ArrayDef)->ArrayLenDef = &ArrayLenDef->getDef();
    const_cast<FuzzInput *>(ArrayLenDef)->ArrayDef = &ArrayDef->getDef();
  }
}

NoInputPtr
FuzzNoInputFactory::generate(std::shared_ptr<const Definition> Def) const {
  assert(Def && "Unexpected Program State");

  // TODO: Type_Unknown is not proper.
  if (!Def->Filters.empty() || !Def->DataType)
    return std::make_shared<FuzzNoInput>(FuzzNoInput::Type_Unknown, Def);

  if (Def->DataType->isFixedLengthArrayPtr() && Def->VarName.empty())
    return std::make_shared<FuzzNoInput>(FuzzNoInput::Type_NoNamedArray, Def);

  if (Def->FilePath && !Def->DataType->isStringType()) {
    return std::make_shared<FuzzNoInput>(FuzzNoInput::Type_Unknown, Def);
  }

  return nullptr;
}

NoInputPtr FuzzNoInputFactory::generate(std::shared_ptr<const Definition> Def,
                                        unsigned AffectedDefID) const {

  return std::make_shared<NoInputByGroup>(Def, AffectedDefID);
}

NoInputPtr FuzzNoInputFactory::generateMeshArrayRelation(
    std::shared_ptr<const Definition> Def) const {

  return std::make_shared<FuzzNoInput>(FuzzNoInput::Type_MeshArrayRelation,
                                       Def);
}

InputPtr
FuzzInputFactory::generate(std::shared_ptr<const Definition> &Def) const {

  assert(Def && "Unexpected Program State");

  auto *T = Def->DataType.get();
  assert(T && "Unexpected Program State");

  // NOTE: This is to change normal pointer to array pointer using array
  // property. Since it is not easy to know pointer is used as array,
  // this attemption looks useful to get more accurate result.
  // However, I think it would not proper approach to change type instance here.
  if (Def->Array) {
    assert(T->isPointerType() && "Unexpected Program State");
    if (T->isNormalPtr())
      T->setPtrKind(Type::PtrKind::PtrKind_Array);
  }

  return std::make_shared<FuzzInput>(Def);
}

void AssignVar::setName(std::string VarName) { this->VarName = VarName; }
void AssignVar::setType(Type &FTGType) {
  const Type *ObjectTy = FTGType.getRealType(/*getArrayPointee=*/true);
  if (ObjectTy->isFixedLengthArrayPtr() && ObjectTy->isStringType()) {
    ObjectTy = ObjectTy->getPointeeType();
    TypeStr = util::stripConstExpr(ObjectTy->getASTTypeName());
  } else
    TypeStr = util::getFuzzInputTypeString(ObjectTy->getASTTypeName());
}
void AssignVar::setNameSpace(std::string NameSpace) {
  this->NameSpace = NameSpace;
}
std::string AssignVar::getTypeStr() { return NameSpace + TypeStr + PtrStr; }
std::string AssignVar::getName() { return VarName; }
void AssignVar::addPtr() {
  PtrStr += '*';
  VarName += 'p';
}
} // end namespace ftg
