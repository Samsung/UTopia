#include "ftg/targetanalysis/TargetLib.h"
#include "ftg/utils/StringUtil.h"
#include "json/json.h"
#include <gtest/gtest.h>

namespace ftg {

TEST(TestTargetLib, toJsonP) {
  auto ExpectedJson = util::strToJson(R"(
    {
      "APIs": [
        "API"
      ]
    })");

  TargetLib TLib({"API"});
  std::vector<EnumConst> Enumerators;
  Enumerators.emplace_back("A", 0);
  ASSERT_EQ(TLib.toJson(), ExpectedJson);
}

TEST(TestTargetLib, fromJsonP) {
  auto TargetLibJson = util::strToJson(R"(
    {
      "APIs": [
        "API"
      ]
    })");

  TargetLib TLib;
  ASSERT_TRUE(TLib.fromJson(TargetLibJson));
  auto APIs = TLib.getAPIs();
  ASSERT_EQ(APIs.size(), 1);
  ASSERT_NE(APIs.find("API"), APIs.end());
}

TEST(TestTargetLib, fromJsonWithEmptyJsonN) {
  auto TargetLibJson = Json::Value();

  TargetLib TLib;
  ASSERT_FALSE(TLib.fromJson(TargetLibJson));
}

TEST(TestTargetLib, fromJsonWithNullN) {
  auto TargetLibJson = Json::nullValue;

  TargetLib TLib;
  ASSERT_FALSE(TLib.fromJson(TargetLibJson));
}

} // namespace ftg
