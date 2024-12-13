//===-- ProtobufDescriptor.cpp - Implementation of ProtobufDescriptor -----===//

#include "ftg/generation/ProtobufDescriptor.h"
#include "ftg/utils/StringUtil.h"

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>

namespace fs = std::experimental::filesystem;

using namespace ftg;

ProtobufDescriptor::ProtobufDescriptor(const std::string &Name,
                                       const std::string &PackageName)
    : Name(Name), FuzzInputDescriptor(*ProtoFile.add_message_type()) {
  ProtoFile.set_name(Name + ".proto");
  ProtoFile.set_syntax("proto3");
  if (!PackageName.empty())
    ProtoFile.set_package(PackageName);
  FuzzInputDescriptor.set_name(Name);
}

bool ProtobufDescriptor::addField(const Type &InputType,
                                  const std::string &FieldName) {
  if (!util::isValidIdentifier(FieldName)) {
    return false;
  }
  return addFieldToDescriptor(FuzzInputDescriptor, FieldName, InputType);
}

void ProtobufDescriptor::genProtoFile(const std::string &OutDir) const {
  google::protobuf::DescriptorPool DescriptorPool;
  auto *FD = DescriptorPool.BuildFile(ProtoFile);
  if (!FD) {
    assert(false && "Failed to generate proto file");
  }
  std::ofstream OFS((fs::path(OutDir) / ProtoFile.name()).string());
  OFS << FD->DebugString();
  OFS.close();
}

std::string ProtobufDescriptor::getHeaderName() const { return Name + ".pb.h"; }

void ProtobufDescriptor::compileProto(const std::string &ProtoDir,
                                      const std::string &FileName) {
  google::protobuf::compiler::CommandLineInterface CLI;
  google::protobuf::compiler::cpp::CppGenerator CPPGenerator;
  CLI.RegisterGenerator("--cpp_out", &CPPGenerator, "");
  std::string ProjectRoot = "--proto_path=" + ProtoDir;
  std::string Output = "--cpp_out=" + ProtoDir;

  const char *Argv[] = {"protoc", ProjectRoot.c_str(), Output.c_str(),
                        FileName.c_str()};

  if (CLI.Run(4, Argv) != 0) {
    assert(false && "Compile Protobuf Failed");
  }
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const Type &T) {
  bool Array = false;
  const auto *BaseType = &T;
  if (BaseType->isFixedLengthArrayPtr() && !BaseType->isStringType()) {
    Array = true;
    BaseType = BaseType->getPointeeType();
  }
  google::protobuf::FieldDescriptorProto *Field = nullptr;
  if (BaseType->isPointerType()) {
    Field = addFieldToDescriptorPointerType(Desc, FieldName, *BaseType);
  } else if (BaseType->isEnumType()) {
    Field = addFieldToDescriptorEnumType(Desc, FieldName, *BaseType);
  } else if (BaseType->isPrimitiveType()) {
    Field = addFieldToDescriptorPrimitiveType(Desc, FieldName, *BaseType);
  }
  if (Field && Array)
    Field->set_label(google::protobuf::FieldDescriptorProto::LABEL_REPEATED);
  return Field;
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptorPrimitiveType(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const Type &T) {
  assert(T.isPrimitiveType() && "Unexpected Program State");
  google::protobuf::FieldDescriptorProto_Type FieldType;
  if (T.isIntegerType()) {
    if (T.isBoolean())
      FieldType = google::protobuf::FieldDescriptorProto::TYPE_BOOL;
    else if (T.getTypeSize() == 8) {
      FieldType = T.isUnsigned()
                      ? google::protobuf::FieldDescriptorProto::TYPE_UINT64
                      : google::protobuf::FieldDescriptorProto::TYPE_SINT64;
    } else {
      FieldType = T.isUnsigned()
                      ? google::protobuf::FieldDescriptorProto::TYPE_UINT32
                      : google::protobuf::FieldDescriptorProto::TYPE_SINT32;
    }
  } else if (T.isFloatType()) {
    FieldType = T.getTypeSize() == 8
                    ? google::protobuf::FieldDescriptorProto::TYPE_DOUBLE
                    : google::protobuf::FieldDescriptorProto::TYPE_FLOAT;
  } else { // Unknown Type
    return nullptr;
  }
  return addFieldToDescriptor(Desc, FieldName, FieldType);
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptorEnumType(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const Type &T) {
  assert(T.isEnumType() && "Unexpected Program State");
  auto EnumName = T.getTypeName();
  // TypeName might contain ':' due to namespace or nested.
  // Replacing them as ':' is not valid character for identifier
  std::replace(EnumName.begin(), EnumName.end(), ':', '_');
  if (Enums.find(EnumName) == Enums.end()) {
    const auto *EnumDef = T.getGlobalDef();
    if (EnumDef == nullptr)
      return nullptr;
    auto *EnumDesc = ProtoFile.add_enum_type();
    EnumDesc->set_name(EnumName);
    // First value of protobuf enum should be 0.
    // Set default name as UNKNOWN and overwrite it if target enum has 0 value.
    // Enum value should be globally unique in protobuf.
    // Adding name of enum as prefix to avoid collision.
    auto *DefaultVal = EnumDesc->add_value();
    DefaultVal->set_name(EnumName + "_UNKNOWN");
    DefaultVal->set_number(0);
    std::set<int64_t> EnumValues;
    bool HaveAlias = false;
    for (auto EnumVal : EnumDef->getElements()) {
      auto ElemVal = EnumVal.getValue();
      auto *ValDesc =
          ElemVal == 0 && EnumDesc->value(0).name() == EnumName + "_UNKNOWN"
              ? EnumDesc->mutable_value(0)
              : EnumDesc->add_value();
      ValDesc->set_name(EnumName + "_" + EnumVal.getName() + "_ENUM");
      ValDesc->set_number(ElemVal);
      auto Result = EnumValues.insert(ElemVal);
      if (!Result.second)
        HaveAlias = true;
    }
    if (HaveAlias)
      EnumDesc->mutable_options()->set_allow_alias(true);
    Enums.emplace(EnumName);
  }
  return addFieldToDescriptor(Desc, FieldName,
                              google::protobuf::FieldDescriptorProto::TYPE_ENUM,
                              EnumName);
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptorPointerType(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const Type &T) {
  assert(T.isPointerType() && "Unexpected Program State");
  if (!T.isStringType())
    return nullptr;
  return addFieldToDescriptor(
      Desc, FieldName, google::protobuf::FieldDescriptorProto::TYPE_BYTES);
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    google::protobuf::FieldDescriptorProto_Type Type,
    const std::string &TypeName) {
  auto *Field = Desc.add_field();
  Field->set_name(FieldName);
  Field->set_number(Desc.field_size());
  Field->set_type(Type);
  if (!TypeName.empty())
    Field->set_type_name(TypeName);
  return Field;
}
