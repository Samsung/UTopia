//===-- TestProtobufMutator.cpp - Unit tests for ProtobufMutator ----------===//

#include "TestHelper.h"
#include "ftg/generation/ProtobufMutator.h"
#include <experimental/filesystem>
#include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;
using namespace ftg;

class TestProtobufMutator : public testing::Test {
protected:
  ProtobufMutator Mutator;

  void verifyGenerateResult() {
    auto TmpDir = fs::temp_directory_path();
    Mutator.genEntry(TmpDir);
    verifyFiles({TmpDir / Mutator.DefaultEntryName,
                 TmpDir / (Mutator.DescriptorName + ".proto")});
  }
};

using TestProtobufMutatorDeathTest = TestProtobufMutator;

TEST_F(TestProtobufMutator, IntP) {
  auto IntT = std::make_shared<IntegerType>();
  IntT->setASTTypeName("int");
  auto Def = std::make_shared<Definition>();
  Def->DataType = IntT;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, IntArrP) {
  auto ArrT = std::make_shared<PointerType>();
  auto ArrInfo = std::make_shared<ArrayInfo>();
  ArrInfo->setMaxLength(10);
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrT->setArrayInfo(ArrInfo);
  ArrT->setPtrKind(PointerType::PtrKind_Array);
  ArrT->setASTTypeName("int *");
  auto IntT = std::make_shared<IntegerType>();
  IntT->setASTTypeName("int");
  ArrT->setPointeeType(IntT);
  auto Def = std::make_shared<Definition>();
  Def->DataType = ArrT;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, FilePathP) {
  auto StrT = std::make_shared<PointerType>();
  StrT->setASTTypeName("char *");
  StrT->setPtrKind(PointerType::PtrKind_String);
  auto Def = std::make_shared<Definition>();
  Def->DataType = StrT;
  Def->FilePath = true;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, EmptyInputN) { verifyGenerateResult(); }

TEST_F(TestProtobufMutatorDeathTest, StructN) {
  // Struct is not supported currently
  auto Def = std::make_shared<Definition>();
  Def->DataType = std::make_shared<StructType>();
  FuzzInput Input(Def);
  ASSERT_DEATH(Mutator.addInput(Input), "");
}

TEST_F(TestProtobufMutatorDeathTest, VoidN) {
  auto Def = std::make_shared<Definition>();
  Def->DataType = std::make_shared<VoidType>();
  FuzzInput Input(Def);
  ASSERT_DEATH(Mutator.addInput(Input), "");
}

TEST_F(TestProtobufMutatorDeathTest, FunctionN) {
  auto Def = std::make_shared<Definition>();
  Def->DataType = std::make_shared<FunctionType>();
  FuzzInput Input(Def);
  ASSERT_DEATH(Mutator.addInput(Input), "");
}
