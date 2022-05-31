#include "ftg/targetanalysis/TargetLibExportUtil.h"
#include "ftg/targetanalysis/ParamReport.h"
#include "ftg/type/Type.h"
#include "google/protobuf/util/json_util.h"
#include "targetpb/targetpb.pb.h"

namespace ftg {

namespace {

targetpb::TargetLib toProtobuf(TargetLib &Src);
targetpb::GlobalDef toProtobuf(GlobalDef &Src);
targetpb::GlobalDef toProtobuf(Function &Src);
targetpb::GlobalDef toProtobuf(Struct &Src);
targetpb::GlobalDef toProtobuf(Enum &Src);
targetpb::TypedElem toProtobuf(TypedElem &Src);
targetpb::TypedElem toProtobuf(Param &Src);
targetpb::Type toProtobuf(const Type &Src);
targetpb::PrimitiveType toProtobuf(const PrimitiveType &Src);
targetpb::IntegerType toProtobuf(const IntegerType &Src);
targetpb::PointerType toProtobuf(const PointerType &Src);
targetpb::ArrayInfo toProtobuf(const ArrayInfo &Src);
targetpb::DefinedType toProtobuf(const DefinedType &Src);
targetpb::EnumType toProtobuf(const EnumType &Src);
targetpb::FunctionReport toProtobuf(FunctionReport &Report);
targetpb::ParamReport toProtobuf(ParamReport &Report);
targetpb::Stats toProtobuf(TargetLib &Src, std::set<std::string> APIs);

targetpb::TargetLib toProtobuf(TargetLib &Src) {
  targetpb::TargetLib Result;
  for (auto Struct : Src.getStructMap())
    Result.add_structs()->CopyFrom(toProtobuf(*Struct.second));
  for (auto Enum : Src.getEnumMap())
    Result.add_enums()->CopyFrom(toProtobuf(*Enum.second));
  for (auto Function : Src.getFunctionMap())
    Result.add_functions()->CopyFrom(toProtobuf(*Function.second));
  for (auto typedefPair : Src.getTypedefMap())
    Result.mutable_typedefmap()->insert(
        {typedefPair.first, typedefPair.second});
  for (auto FunctionReport : Src.getFunctionReportMap())
    Result.add_functionreports()->CopyFrom(toProtobuf(*FunctionReport.second));
  Result.mutable_stats()->CopyFrom(toProtobuf(Src, Src.getAPIs()));
  return Result;
}

targetpb::GlobalDef toProtobuf(GlobalDef &Src) {
  targetpb::GlobalDef Result;
  Result.set_m_typeid(static_cast<targetpb::GlobalDef_TypeID>(Src.getKind()));
  Result.set_m_name(Src.getName());
  Result.set_m_namedemangled(Src.getNameDemangled());
  return Result;
}

targetpb::GlobalDef toProtobuf(Function &Src) {
  auto Result = toProtobuf(llvm::cast<GlobalDef>(Src));
  auto *Func = Result.mutable_function();
  assert(Func && "Unexpected Program State");
  Func->set_m_isvariadic(Src.isVariadic());
  for (auto &Param : Src.getParams()) {
    assert(Param && "Unexpected Program State");
    auto *AddArg = Func->add_m_args();
    assert(AddArg && "Unexpected Program State");
    AddArg->CopyFrom(toProtobuf(*Param));
  }
  return Result;
}

targetpb::GlobalDef toProtobuf(Struct &Src) {
  targetpb::GlobalDef Result = toProtobuf(llvm::cast<GlobalDef>(Src));
  targetpb::Struct *S = Result.mutable_struct_();
  assert(S && "Unexpected Program State");
  for (auto &Field : Src.getFields()) {
    assert(Field && "Unexpected Program State");
    auto *AddField = S->add_m_fields();
    assert(AddField && "Unexpected Program State");
    AddField->CopyFrom(toProtobuf(*Field));
  }
  return Result;
}

targetpb::GlobalDef toProtobuf(Enum &Src) {
  targetpb::GlobalDef Result = toProtobuf(llvm::cast<GlobalDef>(Src));
  targetpb::Enum *E = Result.mutable_enum_();
  assert(E && "Unexpected Program State");
  for (auto Element : Src.getElements()) {
    assert(Element && "Unexpected Program State");
    auto *AddEnum = E->add_m_consts();
    assert(AddEnum && "Unexpected Program State");
    AddEnum->set_name(Element->getName());
    AddEnum->set_value(Element->getType());
  }
  return Result;
}

targetpb::TypedElem toProtobuf(TypedElem &Src) {
  targetpb::TypedElem Result;
  Result.set_m_typeid(static_cast<targetpb::TypedElem_TypeID>(Src.getKind()));
  Result.set_m_index(Src.getIndex());
  Result.set_m_varname_ast(Src.getVarName());
  Result.mutable_elemtype()->CopyFrom(toProtobuf(Src.getType()));
  return Result;
}

targetpb::TypedElem toProtobuf(Param &Src) {
  targetpb::TypedElem Result = toProtobuf(llvm::cast<TypedElem>(Src));
  targetpb::Param *P = Result.mutable_param();
  assert(P && "Unexpected Program State");
  P->set_ptrdepth(Src.getPtrDepth());
  return Result;
}

targetpb::Type toProtobuf(const Type &Src) {
  targetpb::Type Result;
  Result.set_m_typeid(static_cast<targetpb::Type_TypeID>(Src.getKind()));
  Result.set_m_typename_ast(Src.getASTTypeName());
  Result.set_m_namespace(Src.getNameSpace());
  Result.set_m_typesize(Src.getTypeSize());
  if (auto V = llvm::dyn_cast<PrimitiveType>(&Src))
    Result.mutable_primitivetype()->CopyFrom(toProtobuf(*V));
  else if (auto V = llvm::dyn_cast<PointerType>(&Src))
    Result.mutable_pointertype()->CopyFrom(toProtobuf(*V));
  else if (auto V = llvm::dyn_cast<DefinedType>(&Src))
    Result.mutable_definedtype()->CopyFrom(toProtobuf(*V));
  return Result;
}

targetpb::PrimitiveType toProtobuf(const PrimitiveType &Src) {
  targetpb::PrimitiveType Result;
  Result.set_m_typeid(
      static_cast<targetpb::PrimitiveType_TypeID>(Src.getKind()));
  if (auto *V = llvm::dyn_cast<IntegerType>(&Src))
    Result.mutable_integertype()->CopyFrom(toProtobuf(*V));
  return Result;
}

targetpb::IntegerType toProtobuf(const IntegerType &Src) {
  targetpb::IntegerType Rseult;
  Rseult.set_isunsigned(Src.isUnsigned());
  Rseult.set_isboolean(Src.isBoolean());
  Rseult.set_ischaracter(Src.isAnyCharacter());
  return Rseult;
}

targetpb::PointerType toProtobuf(const PointerType &Src) {
  targetpb::PointerType Result;
  const auto *PointeeT = Src.getPointeeType();
  assert(PointeeT && "Unexpected Program State");
  Result.mutable_m_pointeetype()->CopyFrom(toProtobuf(*PointeeT));
  Result.set_kind(static_cast<targetpb::PointerType_PtrKind>(Src.getPtrKind()));
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

targetpb::DefinedType toProtobuf(const DefinedType &Src) {
  targetpb::DefinedType Result;
  Result.set_m_typeid(static_cast<targetpb::DefinedType_TypeID>(Src.getKind()));
  Result.set_m_typename(Src.getTypeName());
  Result.set_m_istyped(Src.isTyped());
  if (auto V = llvm::dyn_cast<EnumType>(&Src))
    Result.mutable_enumtype()->CopyFrom(toProtobuf(*V));
  return Result;
}

targetpb::EnumType toProtobuf(const EnumType &Src) {
  targetpb::EnumType Result;
  Result.set_isunsigned(Src.isUnsigned());
  Result.set_isboolean(Src.isBoolean());
  return Result;
}

targetpb::FunctionReport toProtobuf(FunctionReport &Report) {
  targetpb::FunctionReport Result;
  Result.set_functionname(Report.getDefinition().getName());
  Result.set_ispublicapi(Report.isPublicAPI());
  Result.set_hascoercedparam(Report.hasCoercedParam());
  Result.set_beginargindex(Report.getBeginArgIndex());
  for (auto Param : Report.getParams()) {
    auto *P = Result.add_params();
    assert(P && "Unexpected Program State");
    P->CopyFrom(toProtobuf(*Param));
  }
  return Result;
}

targetpb::ParamReport toProtobuf(ParamReport &Report) {
  targetpb::ParamReport Result;
  Result.set_direction(Report.getDirection());
  Result.set_isarray(Report.isArray());
  Result.set_isarraylength(Report.isArrayLen());
  Result.set_allocation(Report.getAllocation());
  Result.set_paramindex(Report.getParamIndex());
  Result.set_haslengthrelatedarg(Report.hasLenRelatedArg());
  Result.set_lenrelatedargno(Report.getLenRelatedArgNo());
  Result.set_isfilepath(Report.isFilePathString());
  Result.set_functionname(Report.getFunctionName());
  for (auto Param : Report.getChildParams()) {
    auto *P = Result.add_childparams();
    assert(P && "Unexpected Program State");
    P->CopyFrom(toProtobuf(*Param));
  }
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

  for (auto &funcMap : Src.getFunctionReportMap()) {
    if (!funcMap.second->isPublicAPI()) {
      continue;
    }
    APICount_Analyzed++;
    for (auto &param : funcMap.second->getParams()) {
      ParamCount_Total++;
      if (param->isArray()) {
        ParamCount_Array++;
      }
      if (param->isArrayLen()) {
        ParamCount_ArrayLen++;
      }
      ArgDir argDir = param->getDirection();
      unsigned InOut = (Dir::Dir_In | Dir::Dir_Out);
      if (argDir == Dir::Dir_In) {
        ParamCount_InputOnly++;
      } else if (argDir == Dir::Dir_Out) {
        ParamCount_OutputOnly++;
      } else if (argDir == Dir::Dir_Unidentified) {
        ParamCount_Unidentified++;
      } else if (InOut == (argDir & InOut)) {
        ParamCount_InOut++;
      } else if (argDir == (Dir::Dir_In | Dir::Dir_Unidentified)) {
        ParamCount_InUnidentified++;
      } else if (argDir == (Dir::Dir_Out | Dir::Dir_Unidentified)) {
        ParamCount_OutUnidentified++;
      } else { // (argDir == Dir::::Dir_NoOp)
        ParamCount_Unidentified++;
      }
      ArgAlloc argAlloc = param->getAllocation();
      unsigned AllocAndFree = (Alloc::Alloc_Address | Alloc::Alloc_Free);
      if (argAlloc == Alloc::Alloc_Size) {
        ParamCount_AllocSize++;
      } else if (argAlloc == Alloc::Alloc_Address) {
        ParamCount_AllocAddr++;
      } else if (argAlloc == Alloc::Alloc_Free) {
        ParamCount_FreeAddr++;
      } else if (argAlloc == AllocAndFree) {
        ParamCount_AllocAndFree++;
      } else if (argAlloc == Alloc::Alloc_None) {
        ParamCount_NoAlloc++;
      } else {
        ParamCount_StrangeAlloc++;
      }
    }
  }

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

} // namespace

std::string toJsonString(TargetLib &Src) { return getAsString(Src); }

std::string toJsonString(Type &Src) { return getAsString(Src); }

} // namespace ftg
