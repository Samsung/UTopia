#include "TestHelper.h"
#include "ftg/type/Type.h"
#include "ftg/utils/json/json.h"
#include <gtest/gtest.h>

namespace ftg {

class TestArrayInfo : public testing::Test {
protected:
  Json::Value strToJson(std::string JsonStr) {
    Json::Value Json;
    std::istringstream Iss(JsonStr);
    Json::CharReaderBuilder Reader;
    Json::parseFromStream(Reader, Iss, &Json, nullptr);
    return Json;
  }
};

TEST_F(TestArrayInfo, ToJsonP) {
  std::string ExpectedJsonStr = R"(
      {
        "Incomplete" : false,
        "LengthType" : 2,
        "MaxLength" : 20
      })";
  Json::Value ExpectedJson = strToJson(ExpectedJsonStr);

  std::unique_ptr<ArrayInfo> ArrInfo = std::make_unique<ArrayInfo>();
  ArrInfo->setIncomplete(false);
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrInfo->setMaxLength(20);

  auto ArrInfoJson = ArrInfo->toJson();
  ASSERT_EQ(ArrInfoJson.toStyledString(), ExpectedJson.toStyledString());
}

TEST_F(TestArrayInfo, ToJsonWithUnsetN) {
  std::string ExpectedJsonStr = R"(
    {
      "Incomplete" : false,
      "LengthType" : 0,
      "MaxLength" : 0
    })";
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ArrInfo->toJson().toStyledString();
  ASSERT_EQ(ArrInfo->toJson().toStyledString(),
            strToJson(ExpectedJsonStr).toStyledString());
}

TEST_F(TestArrayInfo, FromJsonP) {
  std::unique_ptr<ArrayInfo> ExpectedArrInfo = std::make_unique<ArrayInfo>();
  ExpectedArrInfo->setIncomplete(false);
  ExpectedArrInfo->setLengthType(ArrayInfo::FIXED);
  ExpectedArrInfo->setMaxLength(20);

  std::string JsonStr = R"(
      {
        "Incomplete" : false,
        "LengthType" : 2,
        "MaxLength" : 20
      })";
  Json::Value Json = strToJson(JsonStr);

  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_TRUE(ArrInfo->fromJson(Json));
  ASSERT_EQ(ArrInfo->isIncomplete(), ExpectedArrInfo->isIncomplete());
  ASSERT_EQ(ArrInfo->getLengthType(), ExpectedArrInfo->getLengthType());
  ASSERT_EQ(ArrInfo->getMaxLength(), ExpectedArrInfo->getMaxLength());
}

TEST_F(TestArrayInfo, FromJsonWithEmptyJsonN) {
  Json::Value EmptyJson = strToJson("{}");
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_FALSE(ArrInfo->fromJson(EmptyJson));
}

TEST_F(TestArrayInfo, FromJsonWithJsonNullValueN) {
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_FALSE(ArrInfo->fromJson(Json::nullValue));
}

TEST_F(TestArrayInfo, FromJsonIncompleteJsonN) {
  auto ArrInfo = std::make_unique<ArrayInfo>();
  Json::Value IncompleteJson;
  IncompleteJson = strToJson(R"(
      {
        "LengthType" : 0,
        "MaxLength" : 0
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));

  IncompleteJson = strToJson(R"(
      {
        "Incomplete" : false,
        "MaxLength" : 0
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));

  IncompleteJson = strToJson(R"(
      {
        "Incomplete" : false,
        "LengthType" : 0,
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));
}

} // namespace ftg
