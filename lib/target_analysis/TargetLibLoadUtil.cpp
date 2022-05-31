#include "ftg/targetanalysis/TargetLibLoadUtil.h"
#include "ftg/targetanalysis/ParamReport.h"
#include "google/protobuf/util/json_util.h"
#include "targetpb/targetpb.pb.h"

namespace ftg {

namespace {

std::shared_ptr<Type> createType(const targetpb::Type &PB, TypedElem *Parent,
                                 Type *ParentType, TargetLib *TL);

std::shared_ptr<Type> createPointerType(const targetpb::PointerType &PB,
                                        TypedElem *Parent, TargetLib *TL) {
  std::shared_ptr<PointerType> Result;
  Result = std::make_shared<PointerType>();
  Result->setPointeeType(
      createType(PB.m_pointeetype(), Parent, Result.get(), TL));
  Result->setPtrKind(static_cast<PointerType::PtrKind>(PB.kind()));
  if (!Result->isSinglePtr()) {
    auto protoArrInfo = PB.arrinfo();
    auto ArrInfo = std::make_unique<ArrayInfo>();
    ArrInfo->setLengthType(
        static_cast<ArrayInfo::LengthType>(protoArrInfo.m_lengthtype()));
    ArrInfo->setMaxLength(protoArrInfo.m_maxlength());
    ArrInfo->setIncomplete(protoArrInfo.m_isincomplete());
    Result->setArrayInfo(std::move(ArrInfo));
  }
  return Result;
}

std::shared_ptr<Type> createDefinedType(const targetpb::DefinedType &PB,
                                        TargetLib *TL) {
  switch (PB.m_typeid()) {
  case targetpb::DefinedType_TypeID_Struct: {
    auto Result = std::make_shared<StructType>();
    assert(Result && "Unexpected Program State");
    Result->setTyped(PB.m_istyped());
    if (TL)
      Result->setGlobalDef(TL->getStruct(PB.m_typename()));
    Result->setTypeName(Result->getGlobalDef()
                            ? Result->getGlobalDef()->getName()
                            : PB.m_typename());
    return Result;
  }
  case targetpb::DefinedType_TypeID_Union:
    return std::make_shared<UnionType>();
  case targetpb::DefinedType_TypeID_Class:
    return std::make_shared<ClassType>();
  case targetpb::DefinedType_TypeID_Function:
    return std::make_shared<FunctionType>();
  case targetpb::DefinedType_TypeID_Enum: {
    auto Result = std::make_shared<EnumType>();
    Result->setTyped(PB.m_istyped());
    if (TL)
      Result->setGlobalDef(TL->getEnum(PB.m_typename()));
    Result->setTypeName(Result->getGlobalDef()
                            ? Result->getGlobalDef()->getName()
                            : PB.m_typename());
    Result->setUnsigned(PB.enumtype().isunsigned());
    Result->setBoolean(PB.enumtype().isboolean());
    return Result;
  }
  default:
    return std::make_shared<VoidType>();
  }
}

std::shared_ptr<Type> createType(const targetpb::Type &PB, TypedElem *Parent,
                                 Type *ParentType, TargetLib *TL) {
  std::shared_ptr<Type> Result;
  switch (PB.m_typeid()) {
  case targetpb::Type_TypeID_VoidType:
    Result = std::make_shared<VoidType>();
    break;
  case targetpb::Type_TypeID_PrimitiveType:
    switch (PB.primitivetype().m_typeid()) {
    case targetpb::PrimitiveType_TypeID_IntegerType: {
      auto IntType = std::make_shared<IntegerType>();
      IntType->setUnsigned(PB.primitivetype().integertype().isunsigned());
      IntType->setBoolean(PB.primitivetype().integertype().isboolean());
      IntType->setAnyCharacter(PB.primitivetype().integertype().ischaracter());
      Result = IntType;
      break;
    }
    case targetpb::PrimitiveType_TypeID_FloatType:
      Result = std::make_shared<FloatType>();
      break;
    default:
      return std::make_shared<VoidType>();
    }
    break;
  case targetpb::Type_TypeID_PointerType:
    Result = createPointerType(PB.pointertype(), Parent, TL);
    break;
  case targetpb::Type_TypeID_DefinedType:
    Result = createDefinedType(PB.definedtype(), TL);
    break;
  default:
    return std::make_shared<VoidType>();
  }
  Result->setParent(Parent);
  Result->setParentType(ParentType);
  Result->setASTTypeName(PB.m_typename_ast());
  Result->setNameSpace(PB.m_namespace());
  Result->setTypeSize(PB.m_typesize());
  return Result;
}

std::shared_ptr<Enum> createEnum(targetpb::GlobalDef &PB) {
  std::shared_ptr<Enum> Result = std::make_shared<Enum>();
  if (!Result)
    return nullptr;

  Result->setName(PB.m_name());
  auto &Val = PB.enum_();
  Result->setScoped(Val.m_isscoped());
  Result->setScopedUsingClassTag(PB.enum_().m_isscoped());
  for (auto Element : PB.enum_().m_consts()) {
    Result->addElement(std::make_shared<EnumConst>(
        Element.name(), Element.value(), Result.get()));
  }
  return Result;
}

std::shared_ptr<Field> createField(targetpb::TypedElem &PB, Struct *Parent,
                                   TargetLib &TL) {
  std::shared_ptr<Field> Result = std::make_shared<Field>(Parent);
  if (!Result)
    return nullptr;

  Result->setIndex(PB.m_index());
  Result->setVarName(PB.m_varname_ast());
  Result->setType(createType(PB.elemtype(), Result.get(), nullptr, &TL));
  return Result;
}

std::shared_ptr<Struct> createStruct(targetpb::GlobalDef &PB, TargetLib &TL) {
  std::shared_ptr<Struct> Result = std::make_shared<Struct>();
  if (!Result)
    return nullptr;

  Result->setName(PB.m_name());
  auto &Val = PB.struct_();
  for (auto Element : Val.m_fields()) {
    Result->addField(createField(Element, Result.get(), TL));
  }
  return Result;
}

std::shared_ptr<Param> createParam(targetpb::TypedElem &PB, Function *Parent,
                                   TargetLib &TL) {
  std::shared_ptr<Param> Result = std::make_shared<Param>(Parent);
  if (!Result)
    return nullptr;

  Result->setIndex(PB.m_index());
  Result->setVarName(PB.m_varname_ast());
  Result->setType(createType(PB.elemtype(), Result.get(), nullptr, &TL));
  auto Val = PB.param();
  Result->setPtrDepth(Val.ptrdepth());
  return Result;
}

std::shared_ptr<Function> createFunction(targetpb::GlobalDef &PB,
                                         TargetLib &TL) {
  std::shared_ptr<Function> Result = std::make_shared<Function>();
  if (!Result)
    return nullptr;

  Result->setName(PB.m_name());
  auto &Val = PB.function();
  Result->setVariadic(Val.m_isvariadic());
  for (auto Element : Val.m_args())
    Result->addParam(createParam(Element, Result.get(), TL));

  return Result;
}

void addGlobalDefs(targetpb::TargetLib PB, TargetLib &Result) {
  for (auto TypeDef : PB.typedefmap())
    Result.addTypedef(TypeDef);
  for (auto Enum : PB.enums())
    Result.addEnum(createEnum(Enum));
  for (auto Struct : PB.structs())
    Result.addStruct(createStruct(Struct, Result));
  for (auto Function : PB.functions())
    Result.addFunction(createFunction(Function, Result));
}

std::shared_ptr<ParamReport>
createParamReport(const targetpb::ParamReport &Report,
                  std::shared_ptr<Param> Def = nullptr) {
  auto Result = std::make_shared<ParamReport>(
      Report.functionname(), Report.paramindex(), Def.get(), Report.direction(),
      Report.isarray(), Report.isarraylength(), Report.allocation(),
      Report.isfilepath(), Report.haslengthrelatedarg(),
      Report.lenrelatedargno());
  for (auto ChildParam : Report.childparams())
    Result->addChildParam(createParamReport(ChildParam));
  return Result;
}

std::shared_ptr<FunctionReport>
createFunctionReport(const targetpb::FunctionReport &Report, TargetLib &TL) {
  auto *Def = TL.getFunction(Report.functionname());
  auto ParamDefs = Def->getParams();
  auto Result = std::make_shared<FunctionReport>(Def, Report.ispublicapi(),
                                                 Report.hascoercedparam(),
                                                 Report.beginargindex());
  for (auto Param : Report.params()) {
    Result->addParam(createParamReport(Param, ParamDefs[Param.paramindex()]));
  }
  return Result;
}

void addReports(targetpb::TargetLib &PB, TargetLib &Result) {
  for (auto FuncReport : PB.functionreports())
    Result.addFunctionReport(FuncReport.functionname(),
                             createFunctionReport(FuncReport, Result));
}

} // namespace

std::shared_ptr<Type> typeFromJson(const std::string &JsonString,
                                   TargetLib *Report) {
  targetpb::Type PB;
  google::protobuf::util::JsonStringToMessage(JsonString, &PB);
  return createType(PB, nullptr, nullptr, Report);
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
  addReports(PB, *Report);
  return true;
}

std::unique_ptr<TargetLib> TargetLibLoader::takeReport() {
  return std::move(Report);
}

} // namespace ftg
