#include "TestHelper.h"
#include "ftg/type/Type.h"
#include "ftg/utils/StringUtil.h"
#include <gtest/gtest.h>

namespace ftg {

TEST(TestArrayInfo, ToJsonP) {
  std::string ExpectedJsonStr = R"(
      {
        "Incomplete" : false,
        "LengthType" : 2,
        "MaxLength" : 20
      })";
  Json::Value ExpectedJson = util::strToJson(ExpectedJsonStr);

  std::unique_ptr<ArrayInfo> ArrInfo = std::make_unique<ArrayInfo>();
  ArrInfo->setIncomplete(false);
  ArrInfo->setLengthType(ArrayInfo::FIXED);
  ArrInfo->setMaxLength(20);

  auto ArrInfoJson = ArrInfo->toJson();
  ASSERT_EQ(ArrInfoJson.toStyledString(), ExpectedJson.toStyledString());
}

TEST(TestArrayInfo, ToJsonWithUnsetN) {
  std::string ExpectedJsonStr = R"(
    {
      "Incomplete" : false,
      "LengthType" : 0,
      "MaxLength" : 0
    })";
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ArrInfo->toJson().toStyledString();
  ASSERT_EQ(ArrInfo->toJson().toStyledString(),
            util::strToJson(ExpectedJsonStr).toStyledString());
}

TEST(TestArrayInfo, FromJsonP) {
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
  Json::Value Json = util::strToJson(JsonStr);

  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_TRUE(ArrInfo->fromJson(Json));
  ASSERT_EQ(ArrInfo->isIncomplete(), ExpectedArrInfo->isIncomplete());
  ASSERT_EQ(ArrInfo->getLengthType(), ExpectedArrInfo->getLengthType());
  ASSERT_EQ(ArrInfo->getMaxLength(), ExpectedArrInfo->getMaxLength());
}

TEST(TestArrayInfo, FromJsonWithEmptyJsonN) {
  Json::Value EmptyJson = util::strToJson("{}");
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_FALSE(ArrInfo->fromJson(EmptyJson));
}

TEST(TestArrayInfo, FromJsonWithJsonNullValueN) {
  auto ArrInfo = std::make_unique<ArrayInfo>();
  ASSERT_FALSE(ArrInfo->fromJson(Json::nullValue));
}

TEST(TestArrayInfo, FromJsonIncompleteJsonN) {
  auto ArrInfo = std::make_unique<ArrayInfo>();
  Json::Value IncompleteJson;
  IncompleteJson = util::strToJson(R"(
      {
        "LengthType" : 0,
        "MaxLength" : 0
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));

  IncompleteJson = util::strToJson(R"(
      {
        "Incomplete" : false,
        "MaxLength" : 0
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));

  IncompleteJson = util::strToJson(R"(
      {
        "Incomplete" : false,
        "LengthType" : 0,
      })");
  EXPECT_FALSE(ArrInfo->fromJson(IncompleteJson));
}

} // namespace ftg
