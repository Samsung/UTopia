//===-- TestProtobufDescriptor.cpp - Unit tests for ProtobufDescriptor ----===//

#include "TestHelper.h"
#include "ftg/generation/ProtobufDescriptor.h"
#include <experimental/filesystem>
#include <gtest/gtest.h>

namespace fs = std::experimental::filesystem;

using namespace ftg;

class TestProtobufDescriptor : public testing::Test {
protected:
  std::string TestName;
  std::unique_ptr<ProtobufDescriptor> Descriptor;
  fs::path OutDir;
  std::string OutProtoName;

  TestProtobufDescriptor() {
    TestName = testing::UnitTest::GetInstance()->current_test_info()->name();
    Descriptor = std::make_unique<ProtobufDescriptor>(TestName);
    OutDir = fs::temp_directory_path();
    OutProtoName = TestName + ".proto";
  }

  ~TestProtobufDescriptor() { Descriptor.reset(); }

  void verifyProto() {
    Descriptor->genProtoFile(OutDir);
    verifyFile(OutDir / OutProtoName);
  }
};

TEST_F(TestProtobufDescriptor, TestPrimitiveP) {
  IntegerType Int;
  FloatType Float;
  IntegerType Bool;
  Bool.setBoolean(true);
  IntegerType UInt;
  UInt.setUnsigned(true);
  IntegerType Long;
  Long.setTypeSize(8);
  Descriptor->addField(Int, "int_var");
  Descriptor->addField(Float, "float_var");
  Descriptor->addField(Bool, "bool_var");
  Descriptor->addField(UInt, "unsigned_int_var");
  Descriptor->addField(Long, "long_var");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestArrayP) {
  auto Int = std::make_shared<IntegerType>();
  auto ArrInfo = std::make_shared<ArrayInfo>();
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrInfo->setMaxLength(10);
  PointerType ArrPtr;
  ArrPtr.setPointeeType(Int);
  ArrPtr.setPtrKind(PointerType::PtrKind_Array);
  ArrPtr.setArrayInfo(ArrInfo);
  Descriptor->addField(ArrPtr, "int_arr");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestEnumP) {
  EnumType ET1;
  Enum E1;
  E1.setName("EnumWithZero");
  ET1.setTypeName("EnumWithZero");
  for (int Idx = 0; Idx < 3; ++Idx) {
    E1.addElement(
        std::make_shared<EnumConst>("Val" + std::to_string(Idx), Idx, &E1));
  }
  ET1.setGlobalDef(&E1);
  Descriptor->addField(ET1, "Flag1");

  EnumType ET2;
  Enum E2;
  E2.setName("EnumWithoutZero");
  ET2.setTypeName("EnumWithoutZero");
  for (int Idx = 1; Idx < 3; ++Idx) {
    E2.addElement(
        std::make_shared<EnumConst>("Val" + std::to_string(Idx), Idx, &E2));
  }
  ET2.setGlobalDef(&E2);
  Descriptor->addField(ET2, "Flag2");
  Descriptor->addField(ET2, "Flag3");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestEnumWithOutDefN) {
  EnumType ET;
  ASSERT_FALSE(Descriptor->addField(ET, "Flag"));
}

TEST_F(TestProtobufDescriptor, TestStructP) {
  StructType ST;
  Struct S;
  ST.setTypeName("TestStruct");
  S.setName("TestStruct");
  auto Field1 = std::make_shared<Field>(&S);
  Field1->setVarName("Field1");
  Field1->setType(std::make_shared<IntegerType>());
  S.addField(Field1);
  ST.setGlobalDef(&S);
  Descriptor->addField(ST, "Data");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestComplexStructP) {
  auto ST2 = std::make_shared<StructType>();
  Struct S2;
  ST2->setTypeName("NestedStruct");
  S2.setName("NestedStruct");
  auto InnerField = std::make_shared<Field>(&S2);
  InnerField->setVarName("Field");
  InnerField->setType(std::make_shared<IntegerType>());
  S2.addField(InnerField);
  ST2->setGlobalDef(&S2);

  StructType ST;
  Struct S;
  ST.setTypeName("TestStruct");
  S.setName("TestStruct");
  auto Field2 = std::make_shared<Field>(&S);
  Field2->setVarName("Field");
  Field2->setType(ST2);
  S.addField(Field2);
  ST.setGlobalDef(&S);
  Descriptor->addField(ST, "Data");

  StructType Undefined;
  Descriptor->addField(Undefined, "Data2");

  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestStructWithOutDefN) {
  StructType ST;
  ASSERT_FALSE(Descriptor->addField(ST, "Var"));
}

TEST_F(TestProtobufDescriptor, TestCompileP) {
  Descriptor->genProtoFile(OutDir);
  ProtobufDescriptor::compileProto(OutDir, OutProtoName);
  ASSERT_TRUE(fs::exists(OutDir / (TestName + ".pb.h")));
  ASSERT_TRUE(fs::exists(OutDir / (TestName + ".pb.cc")));
}

TEST_F(TestProtobufDescriptor, TestInvalidFieldNameN) {
  IntegerType Int;
  ASSERT_FALSE(Descriptor->addField(Int, "Test::test"));
  ASSERT_FALSE(Descriptor->addField(Int, ""));
}

TEST_F(TestProtobufDescriptor, TestHeaderNameP) {
  ASSERT_EQ(Descriptor->getHeaderName(), "TestHeaderNameP.pb.h");
}
