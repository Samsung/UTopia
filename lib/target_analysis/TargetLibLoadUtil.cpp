#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/type/Type.h"
#include "google/protobuf/util/json_util.h"
#include "targetpb/targetpb.pb.h"

namespace ftg {

namespace {

std::shared_ptr<Type> createType(const targetpb::Type &PB, TargetLib *TL);

std::shared_ptr<Type> createPrimitiveType(const targetpb::PrimitiveType &PB) {

  if (PB.m_typeid() == targetpb::PrimitiveType_TypeID_IntegerType) {
    auto IntType = std::make_shared<Type>(Type::TypeID_Integer);
    IntType->setUnsigned(PB.integertype().isunsigned());
    IntType->setBoolean(PB.integertype().isboolean());
    IntType->setAnyCharacter(PB.integertype().ischaracter());
    return IntType;
  }
  if (PB.m_typeid() == targetpb::PrimitiveType_TypeID_FloatType) {
    return std::make_shared<Type>(Type::TypeID_Float);
  }
  return nullptr;
}

Type::PtrKind toPointerKind(targetpb::PointerType_PtrKind PtrKind) {
  if (PtrKind ==
      targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_SinglePtr)
    return Type::PtrKind::PtrKind_Normal;
  if (PtrKind ==
      targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_Array)
    return Type::PtrKind::PtrKind_Array;
  if (PtrKind ==
      targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_String)
    return Type::PtrKind::PtrKind_String;
  assert(false && "Unexpected Program State");
}

std::shared_ptr<Type> createPointerType(const targetpb::PointerType &PB,
                                        TargetLib *TL) {
  std::shared_ptr<Type> Result = std::make_shared<Type>(Type::TypeID_Pointer);
  Result->setPointeeType(createType(PB.m_pointeetype(), TL));
  Result->setPtrKind(toPointerKind(PB.kind()));
  if (!Result->isNormalPtr()) {
    auto ProtoArrInfo = PB.arrinfo();
    auto ArrInfo = std::make_unique<ArrayInfo>();
    ArrInfo->setLengthType(
        static_cast<ArrayInfo::LengthType>(ProtoArrInfo.m_lengthtype()));
    ArrInfo->setMaxLength(ProtoArrInfo.m_maxlength());
    ArrInfo->setIncomplete(ProtoArrInfo.m_isincomplete());
    Result->setArrayInfo(std::move(ArrInfo));
  }
  return Result;
}

std::shared_ptr<Type> createDefinedType(const targetpb::DefinedType &PB,
                                        TargetLib *TL) {
  if (PB.m_typeid() == targetpb::DefinedType_TypeID_Enum) {
    auto Result = std::make_shared<Type>(Type::TypeID_Enum);
    if (TL)
      Result->setGlobalDef(TL->getEnum(PB.m_typename()));
    Result->setTypeName(Result->getGlobalDef()
                            ? Result->getGlobalDef()->getName()
                            : PB.m_typename());
    Result->setUnsigned(PB.enumtype().isunsigned());
    Result->setBoolean(PB.enumtype().isboolean());
    return Result;
  }
  return nullptr;
}

std::shared_ptr<Type> createType(const targetpb::Type &PB, TargetLib *TL) {
  std::shared_ptr<Type> Result;
  switch (PB.m_typeid()) {
  case targetpb::Type_TypeID_PrimitiveType:
    Result = createPrimitiveType(PB.primitivetype());
    break;
  case targetpb::Type_TypeID_PointerType:
    Result = createPointerType(PB.pointertype(), TL);
    break;
  case targetpb::Type_TypeID_DefinedType:
    Result = createDefinedType(PB.definedtype(), TL);
    break;
  default:
    return nullptr;
  }
  Result->setASTTypeName(PB.m_typename_ast());
  Result->setNameSpace(PB.m_namespace());
  Result->setTypeSize(PB.m_typesize());
  return Result;
}

std::shared_ptr<Enum> createEnum(targetpb::GlobalDef &PB) {
  std::vector<EnumConst> Enumerators;
  for (auto Element : PB.enum_().m_consts()) {
    Enumerators.emplace_back(Element.name(), Element.value());
  }
  auto Result = std::make_shared<Enum>(PB.m_name(), Enumerators);
  if (!Result)
    return nullptr;
  return Result;
}

void addGlobalDefs(const targetpb::TargetLib &PB, TargetLib &Result) {
  for (auto TypeDef : PB.typedefmap())
    Result.addTypedef(TypeDef);
  for (auto Enum : PB.enums())
    Result.addEnum(createEnum(Enum));
}

} // namespace

std::shared_ptr<Type> typeFromJson(const std::string &JsonString,
                                   TargetLib *Report) {
  targetpb::Type PB;
  google::protobuf::util::JsonStringToMessage(JsonString, &PB);
  return createType(PB, Report);
}

TargetLibLoader::TargetLibLoader() : Report(std::make_unique<TargetLib>()) {}

bool TargetLibLoader::load(const std::string &JsonString) {
  if (!Report)
    return false;

  targetpb::TargetLib PB;
  google::protobuf::util::JsonParseOptions Options;
  Options.ignore_unknown_fields = true;
  if (!google::protobuf::util::JsonStringToMessage(JsonString, &PB, Options)
           .ok())
    return false;
  addGlobalDefs(PB, *Report);
  return true;
}

std::unique_ptr<TargetLib> TargetLibLoader::takeReport() {
  return std::move(Report);
}

} // namespace ftg
