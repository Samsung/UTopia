//===-- ProtobufDescriptor.cpp - Implementation of ProtobufDescriptor -----===//

#include "ftg/generation/ProtobufDescriptor.h"
#include "ftg/utils/StringUtil.h"

#include <experimental/filesystem>
#include <fstream>
#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>

namespace fs = std::experimental::filesystem;

using namespace ftg;

ProtobufDescriptor::ProtobufDescriptor(std::string Name)
    : Name(Name), FuzzInputDescriptor(*ProtoFile.add_message_type()) {
  ProtoFile.set_name(Name + ".proto");
  ProtoFile.set_syntax("proto3");
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
  auto *BaseType = &T;
  if (BaseType->isFixedLengthArrayPtr() && !BaseType->isStringType()) {
    Array = true;
    BaseType = &BaseType->getPointeeType();
  }
  google::protobuf::FieldDescriptorProto *Field = nullptr;
  if (auto *Ptr = llvm::dyn_cast<PointerType>(BaseType)) {
    Field = addFieldToDescriptor(Desc, FieldName, *Ptr);
  } else if (auto *S = llvm::dyn_cast<StructType>(BaseType)) {
    Field = addFieldToDescriptor(Desc, FieldName, *S);
  } else if (auto *E = llvm::dyn_cast<EnumType>(BaseType)) {
    Field = addFieldToDescriptor(Desc, FieldName, *E);
  } else if (auto *P = llvm::dyn_cast<PrimitiveType>(BaseType)) {
    Field = addFieldToDescriptor(Desc, FieldName, *P);
  }
  if (Field && Array)
    Field->set_label(google::protobuf::FieldDescriptorProto::LABEL_REPEATED);
  return Field;
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const PrimitiveType &T) {
  google::protobuf::FieldDescriptorProto_Type FieldType;
  if (auto *IT = llvm::dyn_cast<IntegerType>(&T)) {
    if (IT->isBoolean())
      FieldType = google::protobuf::FieldDescriptorProto::TYPE_BOOL;
    else if (IT->getTypeSize() == 8) {
      FieldType = IT->isUnsigned()
                      ? google::protobuf::FieldDescriptorProto::TYPE_UINT64
                      : google::protobuf::FieldDescriptorProto::TYPE_INT64;
    } else {
      FieldType = IT->isUnsigned()
                      ? google::protobuf::FieldDescriptorProto::TYPE_UINT32
                      : google::protobuf::FieldDescriptorProto::TYPE_INT32;
    }
  } else if (auto *FT = llvm::dyn_cast<FloatType>(&T)) {
    FieldType = FT->getTypeSize() == 8
                    ? google::protobuf::FieldDescriptorProto::TYPE_DOUBLE
                    : google::protobuf::FieldDescriptorProto::TYPE_FLOAT;
  } else { // Unknown Type
    return nullptr;
  }
  return addFieldToDescriptor(Desc, FieldName, FieldType);
}

// TODO: Check if name of struct/enum is unique
//  and doesn't containing invalid char
google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const EnumType &T) {
  auto EnumName = T.getTypeName();
  if (Enums.find(EnumName) == Enums.end()) {
    auto *EnumDef = T.getGlobalDef();
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
    for (auto EnumVal : EnumDef->getElements()) {
      auto *ValDesc = EnumVal->getType() == 0 ? EnumDesc->mutable_value(0)
                                              : EnumDesc->add_value();
      ValDesc->set_name(EnumName + "_" + EnumVal->getName());
      ValDesc->set_number(EnumVal->getType());
    }
    Enums.emplace(EnumName);
  }
  return addFieldToDescriptor(Desc, FieldName,
                              google::protobuf::FieldDescriptorProto::TYPE_ENUM,
                              EnumName);
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const StructType &T) {
  auto StructName = T.getTypeName();
  if (Structs.find(StructName) == Structs.end()) {
    auto *StructDef = T.getGlobalDef();
    if (StructDef == nullptr)
      return nullptr;
    auto *StructDesc = ProtoFile.add_message_type();
    StructDesc->set_name(StructName);
    for (auto StructField : StructDef->getFields())
      addFieldToDescriptor(*StructDesc, StructField->getVarName(),
                           StructField->getType());
    Structs.emplace(StructName);
  }
  return addFieldToDescriptor(
      Desc, FieldName, google::protobuf::FieldDescriptorProto::TYPE_MESSAGE,
      StructName);
}

google::protobuf::FieldDescriptorProto *
ProtobufDescriptor::addFieldToDescriptor(
    google::protobuf::DescriptorProto &Desc, const std::string &FieldName,
    const PointerType &T) {
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
