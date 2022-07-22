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

const std::string ProtobufMutator::DescriptorNameSpace = "AutoFuzz";
const std::string ProtobufMutator::DescriptorName = "FuzzArgsProfile";
const std::string ProtobufMutator::DescriptorObjName = "autofuzz_mutation";

ProtobufMutator::ProtobufMutator()
    : InputMutator(), Descriptor(DescriptorName, DescriptorNameSpace) {
  addHeader(R"("libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h")");
  addHeader('"' + Descriptor.getHeaderName() + '"');
}

void ProtobufMutator::addInput(FuzzInput &Input) {
  FuzzVarDecls.emplace_back(genFuzzVarDecl(Input));
  FuzzVarAssigns.emplace_back(genFuzzVarAssign(Input));
  if (!Input.getDef().ArrayLen) {
    assert(Input.getFTGType().getRealType() && "Unexpected Program State");
    Descriptor.addField(*Input.getFTGType().getRealType(),
                        Input.getProtoVarName());
  }
}

void ProtobufMutator::genEntry(const std::string &OutDir,
                               const std::string &FileName) {
  std::string Code;
  // Headers
  for (const auto &Header : getHeaders())
    Code += SrcGenerator::genIncludeStmt(Header);

  Code += R"(extern "C" {)";
  // Fuzz var declaration
  for (auto FuzzVarDecl : FuzzVarDecls)
    Code += FuzzVarDecl;
  Code += "}";

  // Fuzzer main
  Code += "DEFINE_PROTO_FUZZER(const " + DescriptorNameSpace +
          "::" + DescriptorName + " &" + DescriptorObjName + ") {";

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
  if (Input.getFTGType().getRealType() &&
      Input.getFTGType().getRealType()->isFixedLengthArrayPtr())
    Code += SrcGenerator::genDeclStmt(
        "unsigned", util::getAvasFuzzSizeVarName(FuzzVarName));
  return Code;
}

std::string ProtobufMutator::genFuzzVarAssign(FuzzInput &FuzzingInput) {
  std::string Code;
  // Use intermediate LocalVar for handling pointers
  auto LocalVarName = FuzzingInput.getProtoVarName();
  auto FuzzVarName = FuzzingInput.getFuzzVarName();
  const auto *MutationType = FuzzingInput.getFTGType().getRealType();

  assert(MutationType && "Unexpected Program State");

  // LocalVar declaration
  Code += SrcGenerator::genDeclStmt(*MutationType, LocalVarName);

  // Copy mutation result to LocalVar
  if (MutationType->isPointerType()) {
    Code += copyMutationPointerType(*MutationType, FuzzingInput);
  } else if (MutationType->isEnumType()) {
    Code += copyMutationEnumType(*MutationType, FuzzingInput);
  } else if (MutationType->isPrimitiveType()) {
    Code += copyMutationPrimitiveType(*MutationType, FuzzingInput);
  } else {
    assert(false && "Unsupported mutation type");
  }

  // Copy LocalVar to FuzzVar
  auto &FuzzVarType = FuzzingInput.getFTGType();
  if (&FuzzVarType == MutationType) {
    Code += SrcGenerator::genAssignStmt(FuzzVarName, LocalVarName);
  } else { // FuzzVar is pointer type
    auto PtrVarName = LocalVarName;
    auto TypeStr = SrcGenerator::genTypeStr(*MutationType);
    const auto *TypePtr = &FuzzVarType;
    while (MutationType != TypePtr) {
      Code += SrcGenerator::genAssignStmt(TypeStr + "* " + PtrVarName + "p",
                                          "&" + PtrVarName);
      TypePtr = TypePtr->getPointeeType();
      PtrVarName += "p";
      TypeStr += "*";
    }
    Code += SrcGenerator::genAssignStmt(FuzzVarName, PtrVarName);
  }
  return Code;
}

std::string ProtobufMutator::copyMutationPointerType(const Type &MutationType,
                                                     const FuzzInput &Input) {
  assert(MutationType.isPointerType() && "Unexpected Program State");
  std::string Code;
  auto InputDef = Input.getDef();
  auto LocalVarName = Input.getProtoVarName();
  // Base type can not be single ptr Array or String
  auto MutationVal = getProtoVarVal(LocalVarName);
  auto MutationValBegin = MutationVal + ".begin()";
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
    if (MutationType.getPointeeType()->isEnumType()) { // needs type casting
      Code += "for(int i=0;i<" + FuzzArrSize + ";++i) {" + LocalVarName +
              "[i]=static_cast<" +
              MutationType.getPointeeType()->getTypeName() + ">(" +
              MutationVal + "[i]);}";
    } else {
      addHeader("<algorithm>");
      Code += "std::copy(" + MutationValBegin + ", " + MutationValBegin +
              " + " + FuzzArrSize + ", " + LocalVarName + ");";
    }
  } else if (MutationType.isStringType()) {
    auto StrVal = SrcGenerator::genStrToCStr(MutationVal);
    auto OrgTypeName =
        util::stripConstExpr(MutationType.getPointeeType()->getASTTypeName());
    if (OrgTypeName.compare("char") != 0)
      StrVal = "reinterpret_cast<" + OrgTypeName + "*>(" + StrVal + ")";
    Code += SrcGenerator::genAssignStmt(LocalVarName, StrVal);
  }
  return Code;
}

std::string ProtobufMutator::copyMutationPrimitiveType(const Type &MutationType,
                                                       const FuzzInput &Input) {
  assert(MutationType.isPrimitiveType() && "Unexpected Program State");
  std::string Code;
  auto InputDef = Input.getDef();
  auto LocalVarName = Input.getProtoVarName();
  auto MutationVal = getProtoVarVal(LocalVarName);
  if (InputDef.ArrayLen) {
    const auto *RelatedArrayDef = Input.ArrayDef;
    assert(RelatedArrayDef && "Incomplete Arr-ArrLen relationship");
    const auto &RelatedArrType = RelatedArrayDef->DataType;
    assert(RelatedArrType && "Incomplete Arr-ArrLen relationship");
    // NOTE: below condition statements requires order preserving. char [10]
    // type is both a fixed length array ptr and string ptr, however at here
    // it should be handled as a fixed length array ptr.
    if (RelatedArrType->isFixedLengthArrayPtr()) {
      auto ArrLength =
          std::to_string(RelatedArrType.get()->getArrayInfo()->getMaxLength());
      // Mutation array can be larger than fuzz var array
      Code += SrcGenerator::genAssignStmtWithMaxCheck(
          LocalVarName,
          getProtoVarValSize(util::getProtoVarName(RelatedArrayDef->ID)),
          ArrLength);
    }
    else if (RelatedArrType->isStringType()) {
      Code += SrcGenerator::genAssignStmt(
          LocalVarName,
          getProtoVarValSize(util::getProtoVarName(RelatedArrayDef->ID)));
    }
    else
      assert(false && "Incomplete Arr-ArrLen relationship");
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
  addHeader("<fcntl.h>");
  addHeader("<unistd.h>");
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

std::string ProtobufMutator::copyMutationEnumType(const Type &MutationType,
                                                  const FuzzInput &Input) {
  assert(MutationType.isEnumType() && "Unexpected Program State");
  auto LocalVarName = Input.getProtoVarName();
  return SrcGenerator::genAssignStmt(
      LocalVarName, "static_cast<" + MutationType.getTypeName() + ">(" +
                        getProtoVarVal(LocalVarName) + ")");
}

std::string ProtobufMutator::getProtoVarVal(const std::string &VarName) {
  return DescriptorObjName + "." + VarName + "()";
}

std::string ProtobufMutator::getProtoVarValSize(const std::string &VarName) {
  return getProtoVarVal(VarName) + ".size()";
}
