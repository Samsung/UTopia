//===-- ProtobufMutator.cpp - Implementation of ProtobufMutator -----------===//

#include "ftg/generation/ProtobufMutator.h"
#include "ftg/generation/SrcGenerator.h"
#include "ftg/generation/UTModify.h"
#include "ftg/utils/AssignUtil.h"
#include "ftg/utils/FileUtil.h"
#include "clang/Format/Format.h"
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

using namespace ftg;

const std::string ProtobufMutator::DescriptorName = "FuzzArgsProfile";
const std::string ProtobufMutator::DescriptorObjName = "autofuzz_mutation";

ProtobufMutator::ProtobufMutator()
    : InputMutator(), Descriptor(DescriptorName) {
  Headers.emplace(R"("libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h")");
  Headers.emplace('"' + Descriptor.getHeaderName() + '"');
}

void ProtobufMutator::addInput(FuzzInput &Input) {
  FuzzVarDecls.emplace_back(genFuzzVarDecl(Input));
  FuzzVarAssigns.emplace_back(genFuzzVarAssign(Input));
  if (!Input.getDef().ArrayLen)
    Descriptor.addField(Input.getFTGType().getRealType(),
                        Input.getProtoVarName());
}

void ProtobufMutator::genEntry(const std::string &OutDir,
                               const std::string &FileName) {
  std::string Code;
  // Headers
  for (auto Header : Headers)
    Code += SrcGenerator::genIncludeStmt(Header);

  Code += R"(extern "C" {)";
  // Fuzz var declaration
  for (auto FuzzVarDecl : FuzzVarDecls)
    // TODO: handling _Bool, Seems to be handled at type analysis.
    Code += FuzzVarDecl;
  Code += "}";

  // Fuzzer main
  Code += "DEFINE_PROTO_FUZZER(const " + DescriptorName + " &" +
          DescriptorObjName + ") {";

  // Assign mutation result
  for (auto FuzzVarAssign : FuzzVarAssigns)
    Code += FuzzVarAssign;

  Code += UTModify::UTEntryFuncName + "(); }";

  Code = SrcGenerator::reformat(Code);

  util::saveFile((fs::path(OutDir) / FileName).c_str(), Code.c_str());
  Descriptor.genProtoFile(OutDir);
}

std::string ProtobufMutator::genFuzzVarDecl(FuzzInput &Input) {
  auto FuzzVarName = Input.getFuzzVarName();
  auto Code = SrcGenerator::genDeclStmt(Input.getFTGType(), FuzzVarName, false);
  if (Input.getFTGType().getRealType().isFixedLengthArrayPtr())
    Code += SrcGenerator::genDeclStmt(
        "unsigned", util::getAvasFuzzSizeVarName(FuzzVarName));
  return Code;
}

std::string ProtobufMutator::genFuzzVarAssign(FuzzInput &FuzzingInput) {
  std::string Code;
  // Use intermediate LocalVar for handling pointers
  auto LocalVarName = FuzzingInput.getProtoVarName();
  auto FuzzVarName = FuzzingInput.getFuzzVarName();
  auto &MutationType = FuzzingInput.getFTGType().getRealType();

  // LocalVar declaration
  Code += SrcGenerator::genDeclStmt(MutationType, LocalVarName);

  // Copy mutation result to LocalVar
  if (auto *PtrT = llvm::dyn_cast<PointerType>(&MutationType)) {
    Code += copyMutation(*PtrT, FuzzingInput);
  } else if (auto *CT = llvm::dyn_cast<ClassType>(&MutationType)) {
    Code += copyMutation(*CT, FuzzingInput);
  } else if (auto *ST = llvm::dyn_cast<StructType>(&MutationType)) {
    Code += copyMutation(*ST, FuzzingInput);
  } else if (auto *ET = llvm::dyn_cast<EnumType>(&MutationType)) {
    Code += copyMutation(*ET, FuzzingInput);
  } else if (auto *PrimT = llvm::dyn_cast<PrimitiveType>(&MutationType)) {
    Code += copyMutation(*PrimT, FuzzingInput);
  } else {
    assert(false && "Unsupported mutation type");
  }

  // Copy LocalVar to FuzzVar
  auto &FuzzVarType = FuzzingInput.getFTGType();
  if (&FuzzVarType == &MutationType) {
    Code += SrcGenerator::genAssignStmt(FuzzVarName, LocalVarName);
  } else { // FuzzVar is pointer type
    auto PtrVarName = LocalVarName;
    auto TypeStr = SrcGenerator::genTypeStr(MutationType);
    const auto *TypePtr = &FuzzVarType;
    while (&MutationType != TypePtr) {
      Code += SrcGenerator::genAssignStmt(TypeStr + "* " + PtrVarName + "p",
                                          "&" + PtrVarName);
      TypePtr = &TypePtr->getPointeeType();
      PtrVarName += "p";
      TypeStr += "*";
    }
    Code += SrcGenerator::genAssignStmt(FuzzVarName, PtrVarName);
  }
  return Code;
}

std::string ProtobufMutator::copyMutation(const PointerType &MutationType,
                                          const FuzzInput &Input) {
  std::string Code;
  auto InputDef = Input.getDef();
  auto LocalVarName = Input.getProtoVarName();
  // Base type can not be single ptr Array or String
  auto MutationVal = getProtoVarVal(LocalVarName);
  auto MutationSize = getProtoVarValSize(LocalVarName);
  if (InputDef.FilePath) {
    Code += saveMutationToFile(LocalVarName);
  } else if (MutationType.isFixedLengthArrayPtr()) {
    auto FuzzArrSize = util::getAvasFuzzSizeVarName(Input.getFuzzVarName());
    auto ArrLength =
        std::to_string(MutationType.getArrayInfo()->getMaxLength());
    // Mutation array can be smaller than fuzz var array
    Code += SrcGenerator::genAssignStmtWithMaxCheck(FuzzArrSize, ArrLength,
                                                    MutationSize);
    // Array copy
    Headers.emplace("<algorithm>");
    Code += "std::copy(" + MutationVal + ", " + MutationVal + " + " +
            FuzzArrSize + ", " + LocalVarName + ");";
  } else if (MutationType.isStringType()) {
    Code += SrcGenerator::genAssignStmt(
        LocalVarName, SrcGenerator::genStrToCStr(MutationVal));
  }
  return Code;
}

std::string ProtobufMutator::copyMutation(const PrimitiveType &MutationType,
                                          const FuzzInput &Input) {
  std::string Code;
  auto InputDef = Input.getDef();
  auto LocalVarName = Input.getProtoVarName();
  auto MutationVal = getProtoVarVal(LocalVarName);
  if (InputDef.ArrayLen) {
    auto *RelatedArrayDef = Input.ArrayDef;
    assert(RelatedArrayDef && "Incomplete Arr-ArrLen relationship");
    auto &RelatedArrType = RelatedArrayDef->DataType;
    assert(RelatedArrType && RelatedArrType->isFixedLengthArrayPtr() &&
           "Incomplete Arr-ArrLen relationship");
    auto ArrLength =
        std::to_string(llvm::dyn_cast<PointerType>(RelatedArrType.get())
                           ->getArrayInfo()
                           ->getMaxLength());
    // Mutation array can be larger than fuzz var array
    Code += SrcGenerator::genAssignStmtWithMaxCheck(
        LocalVarName,
        getProtoVarValSize(util::getProtoVarName(RelatedArrayDef->ID)),
        ArrLength);
  } else if (InputDef.BufferAllocSize) {
    // Buffer size must be over 0
    Code += "if (" + MutationVal + " < 1) return;";
    Code +=
        SrcGenerator::genAssignStmt(LocalVarName, MutationVal + " & 0x3fff");
  } else if (InputDef.LoopExit) {
    Code += "if (" + MutationVal + " < 0) return;";
    Code +=
        SrcGenerator::genAssignStmt(LocalVarName, MutationVal + " & 0x3fff");
  } else {
    Code += SrcGenerator::genAssignStmt(LocalVarName, MutationVal);
  }
  return Code;
}

std::string
ProtobufMutator::saveMutationToFile(const std::string &MutationVarName) {
  std::string Code;
  // Headers for file open/write
  Headers.emplace("<fcntl.h>");
  Headers.emplace("<unistd.h>");
  auto MutationVal = getProtoVarVal(MutationVarName);
  auto MutationSize = getProtoVarValSize(MutationVarName);
  auto FuzzFilePathVar = MutationVarName + "_filepath";
  auto FuzzFileDesc = MutationVarName + "_fd";
  Code += "std::string " + FuzzFilePathVar + "(FUZZ_FILEPATH_PREFIX + " +
          MutationVarName + "_file);";
  Code += "int " + FuzzFileDesc + " = open(" + FuzzFilePathVar +
          ".c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);";
  Code += "if (" + FuzzFileDesc + " != -1) {";
  Code += "write(" + FuzzFileDesc + ", " + MutationVal + ".c_str(), " +
          MutationSize + ");";
  Code += "close(" + FuzzFileDesc + ");";
  Code += "}";
  Code += SrcGenerator::genAssignStmt(
      MutationVarName, SrcGenerator::genStrToCStr(FuzzFilePathVar));
  return Code;
}
std::string ProtobufMutator::copyMutation(const ClassType &MutationType,
                                          const FuzzInput &Input) {
  assert(MutationType.isStdStringType() && "Unsupported Class Type");
  auto LocalVarName = Input.getProtoVarName();
  return SrcGenerator::genAssignStmt(LocalVarName,
                                     getProtoVarVal(LocalVarName));
}
std::string ProtobufMutator::copyMutation(const StructType &MutationType,
                                          const FuzzInput &Input) {
  // TODO: Implement struct copy
  assert(false && "Struct is not implemented yet");
}
std::string ProtobufMutator::copyMutation(const EnumType &MutationType,
                                          const FuzzInput &Input) {
  auto LocalVarName = Input.getProtoVarName();
  return SrcGenerator::genAssignStmt(LocalVarName,
                                     getProtoVarVal(LocalVarName));
}

std::string ProtobufMutator::getProtoVarVal(const std::string &VarName) {
  return DescriptorObjName + "." + VarName + "()";
}

std::string ProtobufMutator::getProtoVarValSize(const std::string &VarName) {
  return getProtoVarVal(VarName) + ".size()";
}
