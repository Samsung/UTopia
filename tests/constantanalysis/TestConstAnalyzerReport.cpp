//===-- TestConstAnalyzerReport.cpp - Unit tests for ConstAnalyzerReport --===//

#include "approvals/ApprovalTests.hpp"
#include "ftg/constantanalysis/ConstAnalyzer.h"
#include "ftg/constantanalysis/ConstAnalyzerReport.h"
#include "ftg/utils/json/json.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"

#include <sstream>

using namespace ftg;

TEST(TestConstantAnalyerReport, SerializeP) {
  auto AST =
      clang::tooling::buildASTFromCode("const int IntConst = 1;"
                                       "int IntVar = 1;"
                                       "const double DoubleConst = 1.5;");
  std::vector<clang::ASTUnit *> ASTUnits;
  ASTUnits.push_back(AST.get());
  ConstAnalyzer Analyzer(ASTUnits);
  auto Report = Analyzer.getReport();
  auto JsonReport = Report->toJson();

  auto JsonStr = JsonReport.toStyledString();
  ApprovalTests::Approvals::verify(JsonStr);
}

TEST(TestConstantAnalyerReport, DeserializeP) {
  Json::Value Root;
  // copied from result of serialize test
  const std::string JsonStr = R"(
      {
              "Constant" :
              {
                      "DoubleConst" :
                      {
                              "array" : false,
                              "values" :
                              [
                                      {
                                              "type" : 1,
                                              "value" : "1.500000"
                                      }
                              ]
                      },
                      "IntConst" :
                      {
                              "array" : false,
                              "values" :
                              [
                                      {
                                              "type" : 0,
                                              "value" : "1"
                                      }
                              ]
                      }
              }
      })";
  std::stringstream Stream(JsonStr);
  Stream >> Root;
  ConstAnalyzerReport Report;
  auto Ret = Report.fromJson(Root);
  ASSERT_TRUE(Ret);
  ASSERT_NE(Report.getConstValue("IntConst"), nullptr);
  ASSERT_NE(Report.getConstValue("DoubleConst"), nullptr);
}
