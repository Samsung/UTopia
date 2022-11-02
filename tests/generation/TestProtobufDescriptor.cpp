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
  Type Int(Type::TypeID_Integer);
  Type Float(Type::TypeID_Float);
  Type Bool(Type::TypeID_Integer);
  Bool.setBoolean(true);
  Type UInt(Type::TypeID_Integer);
  UInt.setUnsigned(true);
  Type Long(Type::TypeID_Integer);
  Long.setTypeSize(8);
  Descriptor->addField(Int, "int_var");
  Descriptor->addField(Float, "float_var");
  Descriptor->addField(Bool, "bool_var");
  Descriptor->addField(UInt, "unsigned_int_var");
  Descriptor->addField(Long, "long_var");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestArrayP) {
  auto Int = std::make_shared<Type>(Type::TypeID_Integer);
  auto ArrInfo = std::make_shared<ArrayInfo>();
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrInfo->setMaxLength(10);
  Type ArrPtr(Type::TypeID_Pointer);
  ArrPtr.setPointeeType(Int);
  ArrPtr.setPtrKind(Type::PtrKind::PtrKind_Array);
  ArrPtr.setArrayInfo(ArrInfo);
  Descriptor->addField(ArrPtr, "int_arr");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestEnumP) {
  Type ET1(Type::TypeID_Enum);
  ET1.setTypeName("Namespace::Class::EnumWithZero");
  std::vector<EnumConst> Enumerators;
  for (int Idx = 0; Idx < 3; ++Idx) {
    Enumerators.emplace_back("Val" + std::to_string(Idx), Idx);
  }
  auto E1 = std::make_shared<Enum>("EnumWithZero", Enumerators);
  ET1.setGlobalDef(E1);
  Descriptor->addField(ET1, "Flag1");

  Type ET2(Type::TypeID_Enum);
  ET2.setTypeName("EnumWithoutZero");
  Enumerators.erase(Enumerators.cbegin());
  auto E2 = std::make_shared<Enum>("EnumWithoutZero", Enumerators);
  ET2.setGlobalDef(E2);
  Descriptor->addField(ET2, "Flag2");
  Descriptor->addField(ET2, "Flag3");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestEnumWithAliasP) {
  Type ET1(Type::TypeID_Enum);
  std::vector<EnumConst> Enumerators;
  ET1.setTypeName("EnumWithAlias");
  for (int Idx = 0; Idx < 3; ++Idx) {
    Enumerators.emplace_back("Val" + std::to_string(Idx), 0);
  }
  auto E1 = std::make_shared<Enum>("EnumWithAlias", Enumerators);
  ET1.setGlobalDef(E1);
  Descriptor->addField(ET1, "Flag1");
  verifyProto();
}

TEST_F(TestProtobufDescriptor, TestEnumWithOutDefN) {
  Type ET(Type::TypeID_Enum);
  ASSERT_FALSE(Descriptor->addField(ET, "Flag"));
}

TEST_F(TestProtobufDescriptor, TestCompileP) {
  Descriptor->genProtoFile(OutDir);
  ProtobufDescriptor::compileProto(OutDir, OutProtoName);
  ASSERT_TRUE(fs::exists(OutDir / (TestName + ".pb.h")));
  ASSERT_TRUE(fs::exists(OutDir / (TestName + ".pb.cc")));
}

TEST_F(TestProtobufDescriptor, TestInvalidFieldNameN) {
  Type Int(Type::TypeID_Integer);
  ASSERT_FALSE(Descriptor->addField(Int, "Test::test"));
  ASSERT_FALSE(Descriptor->addField(Int, ""));
}

TEST_F(TestProtobufDescriptor, TestHeaderNameP) {
  ASSERT_EQ(Descriptor->getHeaderName(), "TestHeaderNameP.pb.h");
}
