#include "TestHelper.h"
#include "ftg/analysis/TypeAnalysisReport.h"
#include "ftg/utils/StringUtil.h"

using namespace ftg;

TEST(TestTypeAnalysisReport, constructP) {
  auto Report1 = std::make_unique<TypeAnalysisReport>();
  auto Json1 = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": null
        }
      }
  )");
  ASSERT_EQ(Json1, Report1->toJson());

  std::string Str1 = R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M1","Value": 1},
                {"Name": "M2","Value": 2},
                {"Name": "M3","Value": 3}
              ],
              "Name": "E1"
            }
          }
        }
      }
  )";
  auto Json2 = util::strToJson(Str1);
  auto Report2 = std::make_unique<TypeAnalysisReport>();
  ASSERT_TRUE(Report2->fromJson(Json2));

  Report1 = std::make_unique<TypeAnalysisReport>(Report2.get());
  ASSERT_EQ(Report1->toJson(), Report2->toJson());
}

TEST(TestTypeAnalysisReport, constructN) {
  auto Report1 = std::make_unique<TypeAnalysisReport>();
  auto Report2 = std::make_unique<TypeAnalysisReport>(Report1.get());
  auto Json1 = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": null
        }
      }
  )");
  ASSERT_EQ(Json1, Report2->toJson());
}

TEST(TestTypeAnalysisReport, fromJsonN) {
  auto Report = std::make_unique<TypeAnalysisReport>();
  auto Json1 = util::strToJson(R"({"invalid": "invalid"})");
  ASSERT_FALSE(Report->fromJson(Json1));

  auto Json2 = util::strToJson(R"({"typeanalysis": {"enums": invalid}})");
  ASSERT_FALSE(Report->fromJson(Json2));

  auto Json3 = util::strToJson(R"({"typeanalysis": {"enums":
      ["invalid","invalid"]}})");
  ASSERT_FALSE(Report->fromJson(Json3));

  auto Json4 =
      util::strToJson(R"({"typeanalysis": {"enums": {"invalid": "invalid"}}})");
  ASSERT_DEATH(Report->fromJson(Json4), "");
}

TEST(TestTypeAnalysisReport, plusequalP) {
  std::string Str1 = R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M1","Value": 1},
                {"Name": "M2","Value": 2},
                {"Name": "M3","Value": 3}
              ],
              "Name": "E1"
            }
          }
        }
      }
  )";
  std::string Str2 = R"(
      {
        "typeanalysis": {
          "enums": {
            "E2": {
              "Enumerators": [
                {"Name": "M4","Value": 4},
                {"Name": "M5","Value": 5},
                {"Name": "M6","Value": 6}
              ],
              "Name": "E2"
            }
          }
        }
      }
  )";
  auto Report1 = std::make_unique<TypeAnalysisReport>();
  ASSERT_TRUE(Report1->fromJson(util::strToJson(Str1)));

  auto Report2 = std::make_unique<TypeAnalysisReport>();
  ASSERT_TRUE(Report2->fromJson(util::strToJson(Str2)));

  *Report1 += *Report2;
  auto Answer = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M1","Value": 1},
                {"Name": "M2","Value": 2},
                {"Name": "M3","Value": 3}
              ],
              "Name": "E1"
            },
            "E2": {
              "Enumerators": [
                {"Name": "M4","Value": 4},
                {"Name": "M5","Value": 5},
                {"Name": "M6","Value": 6}
              ],
              "Name": "E2"
            }
          }
        }
      }
  )");
  ASSERT_EQ(Answer, Report1->toJson());
}

TEST(TestTypeAnalysisReport, plusequalN) {
  std::string Str1 = R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M1","Value": 1},
                {"Name": "M2","Value": 2},
                {"Name": "M3","Value": 3}
              ],
              "Name": "E1"
            }
          }
        }
      }
  )";
  std::string Str2 = R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M4","Value": 4},
                {"Name": "M5","Value": 5},
                {"Name": "M6","Value": 6}
              ],
              "Name": "E1"
            }
          }
        }
      }
  )";
  auto Report1 = std::make_unique<TypeAnalysisReport>();
  ASSERT_TRUE(Report1->fromJson(util::strToJson(Str1)));

  auto Report2 = std::make_unique<TypeAnalysisReport>();
  ASSERT_TRUE(Report2->fromJson(util::strToJson(Str2)));

  *Report1 += *Report2;
  auto Answer = util::strToJson(R"(
      {
        "typeanalysis": {
          "enums": {
            "E1": {
              "Enumerators": [
                {"Name": "M1","Value": 1},
                {"Name": "M2","Value": 2},
                {"Name": "M3","Value": 3}
              ],
              "Name": "E1"
            }
          }
        }
      }
  )");
  ASSERT_EQ(Answer, Report1->toJson());
}
