//===-- ProtobufDescriptor.h - Wrapper for protobuf descriptor --*- C++ -*-===//
///
/// \file
/// Defines wrapping class for protobuf message descriptor(.proto) file.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_PROTOBUFDESCRIPTOR_H
#define FTG_GENERATION_PROTOBUFDESCRIPTOR_H

#include "ftg/generation/Fuzzer.h"
#include "ftg/type/Type.h"
#include <google/protobuf/descriptor.pb.h>
#include <string>

namespace ftg {
class ProtobufDescriptor {
public:
  ProtobufDescriptor(std::string Name);

  /// Add field to protobuf message.
  /// \param InputType Data type of field.
  /// \param FieldName Name of field. It should be valid cpp identifier.
  /// \return true if field added successfully else false.
  bool addField(const Type &InputType, const std::string &FieldName);

  /// Create .proto file from current descriptor.
  /// \param OutDir Output directory path.
  void genProtoFile(const std::string &OutDir) const;

  /// \return Header name corresponded with proto file.
  std::string getHeaderName() const;

  /// Compile .proto file to generate cpp source.
  /// \param ProtoDir Path of directory that contains target proto file.
  /// \param FileName File name to compile.
  static void compileProto(const std::string &ProtoDir,
                           const std::string &FileName);

private:
  std::string Name;
  google::protobuf::FileDescriptorProto ProtoFile;
  google::protobuf::DescriptorProto &FuzzInputDescriptor;
  std::set<std::string> Enums;
  std::set<std::string> Structs;

  /// Try adding field to given descriptor.
  /// \return Pointer of added FieldDescriptorProto if succeed else nullptr
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName, const Type &T);
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName, const PrimitiveType &T);
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName, const EnumType &T);
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName, const StructType &T);
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName, const PointerType &T);
  google::protobuf::FieldDescriptorProto *
  addFieldToDescriptor(google::protobuf::DescriptorProto &Desc,
                       const std::string &FieldName,
                       google::protobuf::FieldDescriptorProto_Type Type,
                       const std::string &TypeName = "");
};
} // namespace ftg
#endif // FTG_GENERATION_PROTOBUFDESCRIPTOR_H
