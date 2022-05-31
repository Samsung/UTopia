#include "ftg/utils/FileUtil.h"
#include "ftg/utils/StringUtil.h"
#include <gtest/gtest.h>

namespace ftg {

class CPPClassMethodNameTest : public ::testing::Test {
protected:
  std::string FullMethodName =
      "testNamespace1::testNamespace2::testClass::testMethod";
  std::string MethodOnlyName = "testMethod";
};

TEST_F(CPPClassMethodNameTest, getClassNameWithNamespaceWithFullMethodNameP) {
  std::string ans = "testNamespace1::testNamespace2::testClass";
  std::string ret = ftg::util::getClassNameWithNamespace(FullMethodName);
  ASSERT_EQ(ret, ans);
}

TEST_F(CPPClassMethodNameTest, getClassNameWithNamespaceWithMethodNameOnlyN) {
  std::string ans = "";
  std::string ret = ftg::util::getClassNameWithNamespace(MethodOnlyName);
  ASSERT_EQ(ret, ans);
}

TEST_F(CPPClassMethodNameTest, getMethodNameWithFullMethodNameP) {
  std::string ans = "testMethod";
  std::string ret = ftg::util::getMethodName(FullMethodName);
  ASSERT_EQ(ret, ans);
}

TEST_F(CPPClassMethodNameTest, getMethodNameWithNoNameN) {
  std::string ans = "";
  std::string ret = ftg::util::getMethodName("");
  ASSERT_EQ(ret, ans);
}

TEST(TestRegex, N) { EXPECT_EQ("", util::regex("abcabc", "bc")); }

TEST(TestRegexWithGroup, N) { EXPECT_EQ("", util::regex("abcabc", "b(c)", 1)); }

TEST(TestReplaceStrOnce, N) {
  std::string TestStrA = "abcabcabc";
  std::string TestStrB = TestStrA;
  util::replaceStrOnce(TestStrB, "ba", "");
  EXPECT_EQ(TestStrA, TestStrB);
}

TEST(TestReplaceStrAll, N) {
  std::string TestStrA = "abcabcabc";
  std::string TestStrB = TestStrA;
  util::replaceStrAll(TestStrB, "ba", "");
  EXPECT_EQ(TestStrA, TestStrB);
}

TEST(TestStringUtil, TrimP) {
  const std::string Answer = "A";
  const std::string LBlankStr = " A";
  const std::string RBlankStr = "A ";
  const std::string ABlankStr = " A ";
  EXPECT_EQ(util::ltrim(LBlankStr), Answer);
  EXPECT_EQ(util::rtrim(RBlankStr), Answer);
  EXPECT_EQ(util::trim(ABlankStr), Answer);
  EXPECT_EQ(util::trim(LBlankStr), Answer);
  EXPECT_EQ(util::trim(RBlankStr), Answer);
}

TEST(TestStringUtil, TrimN) {
  const std::string Answer = "A";
  const std::string LBlankStr = " A";
  const std::string RBlankStr = "A ";
  const std::string ABlankStr = " A ";
  EXPECT_NE(util::ltrim(RBlankStr), Answer);
  EXPECT_NE(util::ltrim(ABlankStr), Answer);
  EXPECT_NE(util::rtrim(LBlankStr), Answer);
  EXPECT_NE(util::rtrim(ABlankStr), Answer);
}

TEST(TestGetRelativePath, N) {
  std::string TestStr = "/home/abuild/rpmbuild/FTG_OUT/a.c";
  std::string BaseStr = "home/abuild/rpmbuild/FTG_OUT/";
  EXPECT_EQ("", util::getRelativePath(TestStr, BaseStr));
}

TEST(TestIsHeaderFile, N) {
  EXPECT_FALSE(util::isHeaderFile("a.c"));
  EXPECT_FALSE(util::isHeaderFile("a.cc"));
  EXPECT_FALSE(util::isHeaderFile("a.cpp"));
  EXPECT_FALSE(util::isHeaderFile("a.hpp"));
}

TEST(TestFindFilePaths, N) {
  std::vector<std::string> FilePaths = util::findFilePaths("", ".c");
  EXPECT_EQ(0, FilePaths.size());
}

TEST(TestFindASTFilePaths, N) {
  std::vector<std::string> FilePaths = util::findASTFilePaths("");
  EXPECT_EQ(0, FilePaths.size());
}

TEST(TestFindJsonFilePaths, N) {
  std::vector<std::string> FilePaths = util::findJsonFilePaths("");
  EXPECT_EQ(0, FilePaths.size());
}

TEST(TestSaveFile, N) { EXPECT_FALSE(util::saveFile("", "abc")); }

TEST(TestReadFile, N) { EXPECT_EQ("", util::readFile("")); }

TEST(TestReadFile2, N) { EXPECT_EQ("", util::readFile("abc")); }

TEST(TestMakeDir, N) { EXPECT_NE(0, util::makeDir("")); }

TEST(TestparseJsonFileToJsonValue, N) {
  EXPECT_TRUE(util::parseJsonFileToJsonValue("").empty());
}

TEST(TestparseJsonFileToJsonValue2, N) {
  EXPECT_TRUE(util::parseJsonFileToJsonValue("abc").empty());
}

TEST(TestIsValidIdentifier, N) {
  ASSERT_FALSE(util::isValidIdentifier(""));
  ASSERT_FALSE(util::isValidIdentifier("0Var"));
  ASSERT_FALSE(util::isValidIdentifier("Namespace::Var"));
}

} // namespace ftg
