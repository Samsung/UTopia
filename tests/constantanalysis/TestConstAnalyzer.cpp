//===-- TestConstAnalyzer.cpp - Unit tests for ConstAnalyzer --------------===//

#include "ftg/constantanalysis/ConstAnalyzer.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Tooling/Tooling.h"
#include "gtest/gtest.h"
#include <vector>

using namespace ftg;

TEST(TestConstAnalyzer, CodeWithIntConstP) {
  auto AST = clang::tooling::buildASTFromCode("const int IntConst = 1;");
  auto Constants = ConstAnalyzer::extractConst(*AST);
  ASSERT_EQ(Constants.size(), 1);
  auto ConstPair = Constants.at(0);
  ASSERT_EQ(ConstPair.first, "IntConst");

  // Might need better way to check value
  ASSERT_EQ(ConstPair.second.getValues().size(), 1);
  auto &ValueData = ConstPair.second.getValues().at(0);
  ASSERT_EQ(ValueData.Type, ASTValueData::VType::VType_INT);
  ASSERT_EQ(ValueData.Value, "1");
}

TEST(TestConstAnalyzer, CodeWithDoubleConstP) {
  auto AST =
      clang::tooling::buildASTFromCode("const double DoubleConst = 1.5;");
  auto Constants = ConstAnalyzer::extractConst(*AST);
  ASSERT_EQ(Constants.size(), 1);
  auto ConstPair = Constants.at(0);
  ASSERT_EQ(ConstPair.first, "DoubleConst");

  // Might need better way to check value
  ASSERT_EQ(ConstPair.second.getValues().size(), 1);
  auto &ValueData = ConstPair.second.getValues().at(0);
  ASSERT_EQ(ValueData.Type, ASTValueData::VType::VType_FLOAT);
  ASSERT_EQ(ValueData.Value, "1.500000");
}

TEST(TestConstAnalyzer, CodeWithMultipleConstP) {
  auto AST =
      clang::tooling::buildASTFromCode("const int IntConst = 1;"
                                       "int IntVar = 1;"
                                       "const double DoubleConst = 1.5;");
  auto Constants = ConstAnalyzer::extractConst(*AST);
  ASSERT_EQ(Constants.size(), 2);
  auto ConstPair = Constants.at(0);
  ASSERT_EQ(ConstPair.first, "IntConst");
}

TEST(TestConstAnalyzer, CodeWithoutConstP) {
  auto AST = clang::tooling::buildASTFromCode("int IntVar = 1;"
                                              "double DoubleVar = 1.5;");
  auto Constants = ConstAnalyzer::extractConst(*AST);
  ASSERT_EQ(Constants.size(), 0);
}

TEST(TestConstAnalyzer, OffsetOfExprP) {
  auto AST = clang::tooling::buildASTFromCode(
      "#include <stddef.h>\n"
      "struct ST1 { int E1; int E2; };\n"
      "const unsigned Var = offsetof(struct ST1, E2);\n");
  auto Constants = ConstAnalyzer::extractConst(*AST);
  ASSERT_EQ(Constants.size(), 0);
}

TEST(TestConstAnalyzer, ReportP) {
  auto AST = clang::tooling::buildASTFromCode("const int IntConst = 1;");
  auto AST2 =
      clang::tooling::buildASTFromCode("const double DoubleConst = 1.5;");
  std::vector<clang::ASTUnit *> ASTUnits;
  ASTUnits.push_back(AST.get());
  ASTUnits.push_back(AST2.get());
  ConstAnalyzer Analyzer(ASTUnits);
  auto Report = Analyzer.getReport();

  ASSERT_NE(Report, nullptr);
  auto &ConstReport = static_cast<ConstAnalyzerReport &>(*Report);

  auto *IntConst = ConstReport.getConstValue("IntConst");
  ASSERT_NE(IntConst, nullptr);

  auto *DoubleConst = ConstReport.getConstValue("DoubleConst");
  ASSERT_NE(DoubleConst, nullptr);
}
