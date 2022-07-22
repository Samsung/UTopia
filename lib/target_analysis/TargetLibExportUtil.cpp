#include "ftg/targetanalysis/TargetLibExportUtil.h"
#include "ftg/type/Type.h"
#include "google/protobuf/util/json_util.h"
#include "targetpb/targetpb.pb.h"

namespace ftg {

namespace {

targetpb::TargetLib toProtobuf(TargetLib &Src);
targetpb::GlobalDef toProtobuf(Enum &Src);
targetpb::Type toProtobuf(const Type &Src);
targetpb::PrimitiveType toProtobufPrimitiveType(const Type &Src);
targetpb::IntegerType toProtobufIntegerType(const Type &Src);
targetpb::PointerType toProtobufPointerType(const Type &Src);
targetpb::ArrayInfo toProtobuf(const ArrayInfo &Src);
targetpb::DefinedType toProtobufEnumType(const Type &Src);
targetpb::Stats toProtobuf(TargetLib &Src, std::set<std::string> APIs);
targetpb::Type_TypeID toProtobufType(Type::TypeID TypeID);
targetpb::DefinedType_TypeID toProtobufDefinedType(Type::TypeID TypeID);
targetpb::PrimitiveType_TypeID toProtobufPrimitiveType(Type::TypeID TypeID);
targetpb::PointerType_PtrKind toProtobufPointerKind(const Type &PT);

targetpb::TargetLib toProtobuf(TargetLib &Src) {
  targetpb::TargetLib Result;
  for (auto Enum : Src.getEnumMap())
    Result.add_enums()->CopyFrom(toProtobuf(*Enum.second));
  for (auto typedefPair : Src.getTypedefMap())
    Result.mutable_typedefmap()->insert(
        {typedefPair.first, typedefPair.second});
  Result.mutable_stats()->CopyFrom(toProtobuf(Src, Src.getAPIs()));
  return Result;
}

targetpb::GlobalDef toProtobuf(Enum &Src) {
  targetpb::GlobalDef Result;
  Result.set_m_typeid(targetpb::GlobalDef_TypeID::GlobalDef_TypeID_Enum);
  Result.set_m_name(Src.getName());
  Result.set_m_namedemangled("");
  targetpb::Enum *E = Result.mutable_enum_();
  assert(E && "Unexpected Program State");
  for (auto Element : Src.getElements()) {
    auto *AddEnum = E->add_m_consts();
    assert(AddEnum && "Unexpected Program State");
    AddEnum->set_name(Element.getName());
    AddEnum->set_value(Element.getValue());
  }
  return Result;
}

targetpb::Type toProtobuf(const Type &Src) {
  targetpb::Type Result;
  Result.set_m_typeid(toProtobufType(Src.getKind()));
  Result.set_m_typename_ast(Src.getASTTypeName());
  Result.set_m_namespace(Src.getNameSpace());
  Result.set_m_typesize(Src.getTypeSize());
  if (Src.isPrimitiveType())
    Result.mutable_primitivetype()->CopyFrom(toProtobufPrimitiveType(Src));
  else if (Src.isPointerType())
    Result.mutable_pointertype()->CopyFrom(toProtobufPointerType(Src));
  else if (Src.getKind() == Type::TypeID_Enum)
    Result.mutable_definedtype()->CopyFrom(toProtobufEnumType(Src));
  return Result;
}

targetpb::PrimitiveType toProtobufPrimitiveType(const Type &Src) {
  assert(Src.isPrimitiveType() && "Unexpected Program State");
  targetpb::PrimitiveType Result;
  Result.set_m_typeid(toProtobufPrimitiveType(Src.getKind()));
  if (Src.isIntegerType())
    Result.mutable_integertype()->CopyFrom(toProtobufIntegerType(Src));
  return Result;
}

targetpb::IntegerType toProtobufIntegerType(const Type &Src) {
  assert(Src.isIntegerType() && "Unexpected Program State");
  targetpb::IntegerType Rseult;
  Rseult.set_isunsigned(Src.isUnsigned());
  Rseult.set_isboolean(Src.isBoolean());
  Rseult.set_ischaracter(Src.isAnyCharacter());
  return Rseult;
}

targetpb::PointerType toProtobufPointerType(const Type &Src) {
  assert(Src.isPointerType() && "Unexpected Program State");
  targetpb::PointerType Result;
  const auto *PointeeT = Src.getPointeeType();
  if (PointeeT)
    Result.mutable_m_pointeetype()->CopyFrom(toProtobuf(*PointeeT));
  else
    Result.mutable_m_pointeetype()->CopyFrom(targetpb::Type());
  Result.set_kind(toProtobufPointerKind(Src));
  if (const auto *ArrInfo = Src.getArrayInfo())
    Result.mutable_arrinfo()->CopyFrom(toProtobuf(*ArrInfo));
  return Result;
}

targetpb::ArrayInfo toProtobuf(const ArrayInfo &Src) {
  targetpb::ArrayInfo Result;
  Result.set_m_lengthtype(
      static_cast<targetpb::ArrayInfo_LengthType>(Src.getLengthType()));
  Result.set_m_maxlength(Src.getMaxLength());
  Result.set_m_isincomplete(Src.isIncomplete());
  return Result;
}

targetpb::DefinedType toProtobufEnumType(const Type &Src) {
  assert(Src.isEnumType() && "Unexpected Program State");
  targetpb::DefinedType Result;
  Result.set_m_typeid(toProtobufDefinedType(Src.getKind()));
  Result.set_m_typename(Src.getTypeName());

  targetpb::EnumType EnumResult;
  EnumResult.set_isunsigned(Src.isUnsigned());
  EnumResult.set_isboolean(Src.isBoolean());
  Result.mutable_enumtype()->CopyFrom(EnumResult);
  return Result;
}

targetpb::Stats toProtobuf(TargetLib &Src, std::set<std::string> APIs) {
  unsigned APICount_Total = APIs.size();
  unsigned APICount_Analyzed = 0;
  unsigned ParamCount_Total = 0;
  unsigned ParamCount_Array = 0;
  unsigned ParamCount_ArrayLen = 0;
  unsigned ParamCount_AllocSize = 0;
  unsigned ParamCount_AllocAddr = 0;
  unsigned ParamCount_FreeAddr = 0;
  unsigned ParamCount_AllocAndFree = 0;
  unsigned ParamCount_NoAlloc = 0;
  unsigned ParamCount_StrangeAlloc = 0;
  unsigned ParamCount_InputOnly = 0;
  unsigned ParamCount_OutputOnly = 0;
  unsigned ParamCount_Unidentified = 0;
  unsigned ParamCount_InOut = 0;
  unsigned ParamCount_InUnidentified = 0;
  unsigned ParamCount_OutUnidentified = 0;

  targetpb::Stats ret;
  ret.set_apicount_total(APICount_Total);
  ret.set_apicount_targetlibanalyzed(APICount_Analyzed);
  ret.set_paramcount_total(ParamCount_Total);
  ret.set_paramcount_array(ParamCount_Array);
  ret.set_paramcount_arraylen(ParamCount_ArrayLen);
  ret.set_paramcount_allocsize(ParamCount_AllocSize);
  ret.set_paramcount_allocaddr(ParamCount_AllocAddr);
  ret.set_paramcount_freeaddr(ParamCount_FreeAddr);
  ret.set_paramcount_allocandfree(ParamCount_AllocAndFree);
  ret.set_paramcount_noalloc(ParamCount_NoAlloc);
  ret.set_paramcount_strangealloc(ParamCount_StrangeAlloc);
  ret.set_paramcount_inputonly(ParamCount_InputOnly);
  ret.set_paramcount_outputonly(ParamCount_OutputOnly);
  ret.set_paramcount_unidentified(ParamCount_Unidentified);
  ret.set_paramcount_inout(ParamCount_InOut);
  ret.set_paramcount_inunidentified(ParamCount_InUnidentified);
  ret.set_paramcount_outunidentified(ParamCount_OutUnidentified);
  return ret;
}

template <typename T> std::string getAsString(T &Src) {
  google::protobuf::util::JsonPrintOptions Options;
  Options.add_whitespace = true;
  Options.always_print_primitive_fields = true;
  Options.preserve_proto_field_names = true;

  std::string Result;
  google::protobuf::util::MessageToJsonString(toProtobuf(Src), &Result,
                                              Options);
  return Result;
}

targetpb::Type_TypeID toProtobufType(Type::TypeID TypeID) {
  if (TypeID == Type::TypeID_Integer || TypeID == Type::TypeID_Float)
    return targetpb::Type_TypeID::Type_TypeID_PrimitiveType;
  if (TypeID == Type::TypeID_Pointer)
    return targetpb::Type_TypeID::Type_TypeID_PointerType;
  if (TypeID == Type::TypeID_Enum)
    return targetpb::Type_TypeID::Type_TypeID_DefinedType;
  assert(false && "Unexpected Program State");
}

targetpb::DefinedType_TypeID toProtobufDefinedType(Type::TypeID TypeID) {
  if (TypeID == Type::TypeID_Enum)
    return targetpb::DefinedType_TypeID::DefinedType_TypeID_Enum;
  assert(false && "Unexpected Program State");
}

targetpb::PrimitiveType_TypeID toProtobufPrimitiveType(Type::TypeID TypeID) {
  if (TypeID == Type::TypeID_Integer)
    return targetpb::PrimitiveType_TypeID::PrimitiveType_TypeID_IntegerType;
  if (TypeID == Type::TypeID_Float)
    return targetpb::PrimitiveType_TypeID::PrimitiveType_TypeID_FloatType;
  assert(false && "Unexpected Program State");
}

targetpb::PointerType_PtrKind toProtobufPointerKind(const Type &PT) {
  if (PT.isNormalPtr())
    return targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_SinglePtr;
  if (PT.isArrayPtr())
    return targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_Array;
  if (PT.isStringType())
    return targetpb::PointerType_PtrKind::PointerType_PtrKind_PtrKind_String;

  assert(false && "Unexpected Program State");
}

} // namespace

std::string toJsonString(TargetLib &Src) { return getAsString(Src); }

std::string toJsonString(Type &Src) { return getAsString(Src); }

Json::Value typeToJson(Type *T) {
  Json::Value TypeJson;
  if (!T)
    return Json::Value::null;
  std::string Serialized = toJsonString(*T);
  std::istringstream StrStream(Serialized);
  StrStream >> TypeJson;
  return TypeJson;
}

} // namespace ftg
