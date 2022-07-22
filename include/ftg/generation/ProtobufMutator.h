//===-- ProtobufMutator.h - Mutator using libprotobuf-mutator ---*- C++ -*-===//
///
/// \file
/// Fuzz input mutator which uses libprotobuf-mutator.
///
//===----------------------------------------------------------------------===//

#ifndef FTG_GENERATION_PROTOBUFMUTATOR_H
#define FTG_GENERATION_PROTOBUFMUTATOR_H

#include "ftg/generation/InputMutator.h"
#include "ftg/generation/ProtobufDescriptor.h"

namespace ftg {

class ProtobufMutator : public InputMutator {
public:
  static const std::string DescriptorNameSpace;
  static const std::string DescriptorName;
  ProtobufMutator();
  void addInput(FuzzInput &Input) override;
  void genEntry(const std::string &OutDir,
                const std::string &FileName = DefaultEntryName) override;

private:
  static const std::string DescriptorObjName;
  ProtobufDescriptor Descriptor;
  std::vector<std::string> FuzzVarDecls;
  std::vector<std::string> FuzzVarAssigns;
  /// Generates fuzz var declaration statement.
  std::string genFuzzVarDecl(FuzzInput &Input);

  /// Generates assignment statement that assign mutation result to fuzz var.
  std::string genFuzzVarAssign(FuzzInput &FuzzingInput);

  /// Saves mutation result to File.
  std::string saveMutationToFile(const std::string &MutationVarName);

  /// Gets mutation results from mutator.
  std::string copyMutationPrimitiveType(const Type &MutationType,
                                        const FuzzInput &Input);
  std::string copyMutationPointerType(const Type &MutationType,
                                      const FuzzInput &Input);
  std::string copyMutationEnumType(const Type &MutationType,
                                   const FuzzInput &Input);

  /// Gets mutation value from protobuf object.
  std::string getProtoVarVal(const std::string &VarName);
  /// Gets size of mutation value from protobuf object.
  std::string getProtoVarValSize(const std::string &VarName);
};

} // namespace ftg

#endif // FTG_GENERATION_PROTOBUFMUTATOR_H
