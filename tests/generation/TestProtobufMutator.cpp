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
  auto IntT = std::make_shared<Type>(Type::TypeID_Integer);
  IntT->setASTTypeName("int");
  auto Def = std::make_shared<Definition>();
  Def->DataType = IntT;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, IntArrP) {
  auto ArrT = std::make_shared<Type>(Type::TypeID_Pointer);
  auto ArrInfo = std::make_shared<ArrayInfo>();
  ArrInfo->setMaxLength(10);
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrT->setArrayInfo(ArrInfo);
  ArrT->setPtrKind(Type::PtrKind::PtrKind_Array);
  ArrT->setASTTypeName("int *");
  auto IntT = std::make_shared<Type>(Type::TypeID_Integer);
  IntT->setASTTypeName("int");
  ArrT->setPointeeType(IntT);
  auto Def = std::make_shared<Definition>();
  Def->DataType = ArrT;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, FilePathP) {
  auto StrT = std::make_shared<Type>(Type::TypeID_Pointer);
  StrT->setASTTypeName("char *");
  StrT->setPtrKind(Type::PtrKind::PtrKind_String);
  auto Def = std::make_shared<Definition>();
  Def->DataType = StrT;
  Def->FilePath = true;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, EmptyInputN) { verifyGenerateResult(); }

TEST_F(TestProtobufMutatorDeathTest, NullDataTypeN) {
  auto Def = std::make_shared<Definition>();
  Def->DataType = nullptr;
  ASSERT_DEATH(FuzzInput Input(Def), "");
}

TEST_F(TestProtobufMutator, ArrayLenWhoseArrayIsStringPtrP) {
  auto ArrayT = Type::createCharPointerType();
  auto ArrayD = std::make_shared<Definition>();
  ArrayD->Array = true;
  ArrayD->DataType = ArrayT;
  ArrayD->ID = 0;

  FuzzInput ArrayInput(ArrayD);

  auto ArrayLenT = std::make_shared<Type>(Type::TypeID_Integer);
  ArrayLenT->setASTTypeName("unsigned");

  auto ArrayLenD = std::make_shared<Definition>();
  ArrayLenD->ArrayLen = true;
  ArrayLenD->DataType = ArrayLenT;
  ArrayLenD->ID = 1;

  FuzzInput ArrayLenInput(ArrayLenD);

  ArrayInput.ArrayLenDef = ArrayLenD.get();
  ArrayLenInput.ArrayDef = ArrayD.get();

  Mutator.addInput(ArrayInput);
  Mutator.addInput(ArrayLenInput);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, NoArrayDefInArrayLenInputN) {
  auto ArrayLenT = std::make_shared<Type>(Type::TypeID_Integer);
  ArrayLenT->setASTTypeName("unsigned");

  auto ArrayLenD = std::make_shared<Definition>();
  ArrayLenD->ArrayLen = true;
  ArrayLenD->DataType = ArrayLenT;
  ArrayLenD->ID = 1;

  FuzzInput ArrayLenInput(ArrayLenD);

  ProtobufMutator Mutator;
  ASSERT_DEATH(Mutator.addInput(ArrayLenInput), "");
}

TEST_F(TestProtobufMutator, NoArrayTypeInArrayLenInputN) {
  auto ArrayD = std::make_shared<Definition>();
  ArrayD->Array = true;
  ArrayD->ID = 0;

  auto ArrayLenT = std::make_shared<Type>(Type::TypeID_Integer);
  ArrayLenT->setASTTypeName("unsigned");

  auto ArrayLenD = std::make_shared<Definition>();
  ArrayLenD->ArrayLen = true;
  ArrayLenD->DataType = ArrayLenT;
  ArrayLenD->ID = 1;

  FuzzInput ArrayLenInput(ArrayLenD);
  ArrayLenInput.ArrayDef = ArrayD.get();

  ProtobufMutator Mutator;
  ASSERT_DEATH(Mutator.addInput(ArrayLenInput), "");
}

TEST_F(TestProtobufMutator, ArrayLenWhoseArrayIsNotSupportN) {
  auto IntT = std::make_shared<Type>(Type::TypeID_Integer);
  IntT->setASTTypeName("unsigned");

  auto ArrayT = std::make_shared<Type>(Type::TypeID_Pointer);
  ArrayT->setASTTypeName("int *");
  ArrayT->setPointeeType(IntT);
  ArrayT->setPtrKind(Type::PtrKind::PtrKind_Array);

  auto ArrayD = std::make_shared<Definition>();
  ArrayD->Array = true;
  ArrayD->DataType = ArrayT;
  ArrayD->ID = 0;

  auto ArrayLenT = std::make_shared<Type>(Type::TypeID_Integer);
  ArrayLenT->setASTTypeName("unsigned");

  auto ArrayLenD = std::make_shared<Definition>();
  ArrayLenD->ArrayLen = true;
  ArrayLenD->DataType = ArrayLenT;
  ArrayLenD->ID = 1;

  FuzzInput ArrayLenInput(ArrayLenD);

  ArrayLenInput.ArrayDef = ArrayD.get();

  ProtobufMutator Mutator;

  auto ArrayArrayInfo = std::make_shared<ArrayInfo>();
  ArrayArrayInfo->setLengthType(ArrayInfo::UNLIMITED);
  ArrayT->setArrayInfo(ArrayArrayInfo);
  ASSERT_DEATH(Mutator.addInput(ArrayLenInput), "");

  ArrayArrayInfo->setLengthType(ArrayInfo::VARIABLE);
  ArrayT->setArrayInfo(ArrayArrayInfo);
  ASSERT_DEATH(Mutator.addInput(ArrayLenInput), "");
}

TEST_F(TestProtobufMutator, UCharStrP) {
  auto UCharT = std::make_shared<Type>(Type::TypeID_Integer);
  UCharT->setASTTypeName("const unsigned char");
  UCharT->setAnyCharacter(true);
  UCharT->setTypeSize(1);

  auto StrT = std::make_shared<Type>(Type::TypeID_Pointer);
  StrT->setASTTypeName("const unsigned char *");
  StrT->setPtrKind(Type::PtrKind::PtrKind_String);

  StrT->setPointeeType(UCharT);

  auto StrD = std::make_shared<Definition>();
  StrD->DataType = StrT;

  FuzzInput StrInput(StrD);

  Mutator.addInput(StrInput);
  verifyGenerateResult();
}

TEST_F(TestProtobufMutator, EnumArrayP) {
  auto ArrT = std::make_shared<Type>(Type::TypeID_Pointer);
  auto ArrInfo = std::make_shared<ArrayInfo>();
  ArrInfo->setMaxLength(10);
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrT->setArrayInfo(ArrInfo);
  ArrT->setPtrKind(Type::PtrKind::PtrKind_Array);
  ArrT->setASTTypeName("EnumT *");
  std::vector<EnumConst> Enumerators{{"E1", 0}};
  auto EnumDef = std::make_shared<Enum>("EnumT", Enumerators);
  auto EnumT = std::make_shared<Type>(Type::TypeID_Enum);
  EnumT->setTypeName("EnumT");
  EnumT->setGlobalDef(EnumDef.get());
  ArrT->setPointeeType(EnumT);
  auto Def = std::make_shared<Definition>();
  Def->DataType = ArrT;
  FuzzInput Input(Def);
  Mutator.addInput(Input);
  verifyGenerateResult();
}
