#include "TestHelper.h"
#include "ftg/generation/UTModify.h"
#include "testutil/APIManualLoader.h"
#include "testutil/SourceFileManager.h"

using namespace ftg;
using namespace clang::tooling;
namespace fs = std::experimental::filesystem;

class TestUTModify : public testing::Test {
protected:
  const std::string HeaderContent =
      "#ifndef AUTOFUZZ_H\n"
      "#define AUTOFUZZ_H\n\n"
      "#ifndef AUTOFUZZ\n"
      "#error AUTOFUZZ is not defined. Do not include this header without "
      "AUTOFUZZ Definition.\n"
      "#endif // AUTOFUZZ\n\n"
      "#ifndef FUZZ_FILEPATH_PREFIX\n"
      "#define FUZZ_FILEPATH_PREFIX ./\n"
      "#endif // FUZZ_FILEPATH_PREFIX\n\n"
      "#ifdef __cplusplus\n"
      "#include <exception>\n"
      "#include <ostream>\n"
      "#include <string>\n\n"
      "struct AutofuzzException : public std::exception {\n"
      "  AutofuzzException(const std::string Src) {}\n"
      "  template <typename T>\n"
      "  AutofuzzException &operator<<(const T Rhs) { return *this; }\n"
      "  AutofuzzException &operator<<(std::ostream& (&Src)(std::ostream&)) "
      "{ return *this; }\n"
      "};\n\n"
      "#undef GTEST_PRED_FORMAT2_\n"
      "#define THROW_AUTOFUZZ_EXCEPTION(Msg) throw AutofuzzException(Msg)\n"
      "#define GTEST_PRED_FORMAT2_(pred_format, v1, v2, on_failure) \\\n"
      "  GTEST_ASSERT_(\\\n"
      "    pred_format(#v1, #v2, v1, v2), THROW_AUTOFUZZ_EXCEPTION)\n\n"
      "extern \"C\" {\n"
      "#endif // __cplusplus\n"
      "void enterAutofuzz();\n"
      "#ifdef __cplusplus\n"
      "}\n"
      "#endif // __cplusplus\n"
      "#endif // AUTOFUZZ_H\n";

  std::shared_ptr<Fuzzer> createFuzzer(const GenLoader &Loader) {
    const auto &Unittests = Loader.getInputReport().getUnittests();
    if (Unittests.size() != 1)
      return nullptr;
    return Fuzzer::create(Unittests[0], Loader.getInputReport());
  }

  bool checkNew(std::map<std::string, std::string> Expect,
                const std::map<std::string, std::string> &Result) {
    for (const auto &Iter1 : Result) {
      auto Path = fs::path(Iter1.first).filename().string();
      auto FindIter = Expect.find(Path);
      if (FindIter == Expect.end())
        return false;
      if (Iter1.second != FindIter->second)
        return false;
      Expect.erase(FindIter);
    }
    if (Expect.size() != 0)
      return false;
    return true;
  }

  bool checkMod(std::vector<Replacement> Expect,
                const std::map<std::string, Replacements> &Result) {
    for (const auto &Iter1 : Result) {
      for (const auto &Iter2 : Iter1.second) {
        auto FindResult =
            std::find_if(Expect.begin(), Expect.end(), [&Iter2](const auto &R) {
              return fs::path(Iter2.getFilePath()).filename().string() ==
                         R.getFilePath() &&
                     Iter2.getOffset() == R.getOffset() &&
                     Iter2.getLength() == R.getLength() &&
                     Iter2.getReplacementText() == R.getReplacementText();
            });
        if (FindResult == Expect.end()) {
          return false;
        }
        Expect.erase(FindResult);
      }
    }
    if (Expect.size() != 0)
      return false;
    return true;
  }

  std::unique_ptr<GenLoader>
  createLoader(SourceFileManager &SFM, const std::set<std::string> APINames,
               const std::string TargetName,
               const std::set<std::string> UTNames) const {
    std::vector<std::string> TargetSrcPaths = {SFM.getFilePath(TargetName)};
    auto TAHelper = TestHelperFactory().createTATestHelper(
        SFM.getBaseDirPath(), TargetSrcPaths, CompileHelper::SourceType_CPP);
    if (!TAHelper || !TAHelper->analyze())
      return nullptr;

    std::string TAReportDirPath = SFM.getBaseDirPath() + "/target";
    if (!fs::create_directories(TAReportDirPath))
      return nullptr;

    std::string TAReportPath = TAReportDirPath + "/TA.json";
    if (!TAHelper->dumpReport(TAReportPath))
      return nullptr;

    if (!SFM.addFile(TAReportPath))
      return nullptr;

    std::shared_ptr<APILoader> AL = std::make_shared<APIManualLoader>(APINames);
    if (!AL)
      return nullptr;

    std::vector<std::string> UTPaths;
    for (const auto &UTName : UTNames)
      UTPaths.emplace_back(SFM.getFilePath(UTName));

    auto UAHelper = TestHelperFactory().createUATestHelper(
        SFM.getBaseDirPath(), UTPaths, CompileHelper::SourceType_CPP);
    if (!UAHelper || !UAHelper->analyze(TAReportDirPath, AL, "gtest"))
      return nullptr;

    std::string UAReportPath = SFM.getBaseDirPath() + "/UA.json";
    if (!UAHelper->dumpReport(UAReportPath))
      return nullptr;
    SFM.addFile(UAReportPath);

    auto Loader = std::make_unique<GenLoader>();
    if (!Loader || !Loader->load(TAReportDirPath, SFM.getFilePath("UA.json")))
      return nullptr;

    return Loader;
  }
};

TEST_F(TestUTModify, ModifyP) {
  SourceFileManager SFM;
  ASSERT_TRUE(SFM.createFile("library.h", "extern \"C\" {\n"
                                          "void API_1(int);\n"
                                          "void API_2(void *);\n"
                                          "}"));
  ASSERT_TRUE(SFM.createFile("library.cpp", "#include \"library.h\"\n"
                                            "extern \"C\" {\n"
                                            "void EXTERN_1(int);\n"
                                            "void EXTERN_2(void *);\n"
                                            "void API_1(int P) {\n"
                                            "  EXTERN_1(P);\n"
                                            "}\n"
                                            "void API_2(void *P) {\n"
                                            "  EXTERN_2(P);\n"
                                            "}}"));
  ASSERT_TRUE(SFM.createFile("ut.cpp",
                             "#include \"library.h\"\n"
                             "#include \"gtest/gtest.h\"\n"
                             "int SUB_1(int P);\n"
                             "namespace NS {\n"
                             "int GVar1;\n"
                             "int GVar2[10];\n"
                             "}\n"
                             "int GVar3;\n"
                             "int GVar4[10];\n"
                             "const int GVar5 = 0;\n"
                             "TEST(Test, Test) {\n"
                             "  int Var1;\n"
                             "  int *Var2;\n"
                             "  char *Var3;\n"
                             "  char **Var4;\n"
                             "  const int Var5 = 0;\n"
                             "  int Var6[10];\n"
                             "  int Var7 = 10;\n"
                             "  int Var8[3] = { 0, 1, 2 };\n"
                             "  static int Var9 = 10;\n"
                             "  API_1(Var1);\n"
                             "  API_2(Var2);\n"
                             "  API_2(Var3);\n"
                             "  API_2(Var4);\n"
                             "  API_2(Var6);\n"
                             "  API_1(NS::GVar1);\n"
                             "  API_2(NS::GVar2);\n"
                             "  API_1(GVar3);\n"
                             "  API_2(GVar4);\n"
                             "  API_1(GVar5);\n"
                             "  API_1(Var5);\n"
                             "  API_1(Var7);\n"
                             "  API_2(Var8);\n"
                             "  API_1(Var9);\n"
                             "  API_1(10);\n"
                             "  API_1(SUB_1(GVar3));\n"
                             "}\n"
                             "int main(int argc, char **argv) {\n"
                             "  ::testing::InitGoogleTest(&argc, argv);\n"
                             "  return RUN_ALL_TESTS();\n"
                             "}\n"));
  ASSERT_TRUE(SFM.createFile("ut-1.cpp", "int SUB_1(int P) {\n"
                                         "  static int Var = 10;\n"
                                         "  if (P < 0) return Var++;\n"
                                         "  return 10;\n"
                                         "}\n"));
  auto Loader = createLoader(SFM, {"API_1", "API_2"}, "library.cpp",
                             {"ut.cpp", "ut-1.cpp"});
  ASSERT_TRUE(Loader);

  auto F = createFuzzer(*Loader);
  ASSERT_TRUE(F);

  const std::vector<Replacement> ModAnswers = {
      {"ut-1.cpp", 0, 0,
       "#ifdef __cplusplus\n"
       "extern \"C\" {\n"
       "#endif\n"
       "extern int autofuzz0;\n"
       "extern int autofuzz1;\n"
       "extern unsigned int autofuzz0_flag;\n"
       "#ifdef __cplusplus\n"
       "}\n"
       "#endif\n"},
      {"ut-1.cpp", 41, 0,
       " if (autofuzz0_flag) { autofuzz0_flag = 0; Var = autofuzz0; }"},
      {"ut-1.cpp", 78, 2, "autofuzz1"},
      {"ut.cpp", 0, 0,
       "#include \"library.h\"\n"
       "#include \"gtest/gtest.h\"\n"
       "#include \"autofuzz.h\"\n"
       "#ifdef __cplusplus\n"
       "extern \"C\" {\n"
       "#endif\n"
       "extern int autofuzz2;\n"
       "extern int * autofuzz3;\n"
       "extern unsigned autofuzz3size;\n"
       "extern int autofuzz4;\n"
       "extern int * autofuzz5;\n"
       "extern unsigned autofuzz5size;\n"
       "extern int autofuzz6;\n"
       "extern int autofuzz7;\n"
       "extern char * autofuzz9;\n"
       "extern int autofuzz11;\n"
       "extern int * autofuzz12;\n"
       "extern unsigned autofuzz12size;\n"
       "extern int autofuzz13;\n"
       "extern int * autofuzz14;\n"
       "extern unsigned autofuzz14size;\n"
       "extern int autofuzz15;\n"
       "extern int autofuzz16;\n"
       "unsigned int autofuzz0_flag = 1;\n"
       "unsigned int autofuzz15_flag = 1;\n"
       "void assign_fuzz_input_to_global_autofuzz2();\n"
       "void assign_fuzz_input_to_global_autofuzz3();\n"
       "void assign_fuzz_input_to_global_autofuzz4();\n"
       "void assign_fuzz_input_to_global_autofuzz5();\n"
       "void assign_fuzz_input_to_global_autofuzz6();\n"
       "#ifdef __cplusplus\n"
       "}\n"
       "#endif\n"},
      {"ut.cpp", 133, 10, "int "},
      {"ut.cpp", 183, 0, " = autofuzz7"},
      {"ut.cpp", 210, 0, " = autofuzz9"},
      {"ut.cpp", 229, 10, "int "},
      {"ut.cpp", 246, 1, "autofuzz11"},
      {"ut.cpp", 264, 0,
       " { for (unsigned i=0; i<autofuzz12size; ++i) "
       "{ Var6[i] = autofuzz12[i]; } }"},
      {"ut.cpp", 278, 2, "autofuzz13"},
      {"ut.cpp", 310, 0,
       " { for (unsigned i=0; i<autofuzz14size; ++i) "
       "{ Var8[i] = autofuzz14[i]; } }"},
      {"ut.cpp", 334, 0,
       " if (autofuzz15_flag) { autofuzz15_flag = 0; Var9 = autofuzz15; }"},
      {"ut.cpp", 566, 2, "autofuzz16"},
      {"ut.cpp", 596, 103, ""},
      {"ut.cpp", 700, 0,
       "#ifdef __cplusplus\n"
       "extern \"C\" {\n"
       "#endif\n"
       "void assign_fuzz_input_to_global_autofuzz2() {\n"
       "    NS::GVar1 = autofuzz2;\n"
       "}\n"
       "void assign_fuzz_input_to_global_autofuzz3() {\n"
       "    { for (unsigned i=0; i<autofuzz3size; ++i) "
       "{ NS::GVar2[i] = autofuzz3[i]; } }\n"
       "}\n"
       "void assign_fuzz_input_to_global_autofuzz4() {\n"
       "    GVar3 = autofuzz4;\n"
       "}\n"
       "void assign_fuzz_input_to_global_autofuzz5() {\n"
       "    { for (unsigned i=0; i<autofuzz5size; ++i) "
       "{ GVar4[i] = autofuzz5[i]; } }\n"
       "}\n"
       "void assign_fuzz_input_to_global_autofuzz6() {\n"
       "    GVar5 = autofuzz6;\n"
       "}\n"
       "void enterAutofuzz() {\n"
       "  assign_fuzz_input_to_global_autofuzz2();\n"
       "  assign_fuzz_input_to_global_autofuzz3();\n"
       "  assign_fuzz_input_to_global_autofuzz4();\n"
       "  assign_fuzz_input_to_global_autofuzz5();\n"
       "  assign_fuzz_input_to_global_autofuzz6();\n"
       "  autofuzz0_flag = 1;\n"
       "  autofuzz15_flag = 1;\n"
       "  class AutofuzzTest : public Test_Test_Test {\n"
       "  public:\n"
       "    void runTest() {\n"
       "      try {\n"
       "        SetUpTestCase();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        SetUp();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TestBody();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TearDown();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TearDownTestCase();\n"
       "      } catch (std::exception &E) {}\n"
       "    }\n"
       "  };\n"
       "  AutofuzzTest Fuzzer;\n"
       "  Fuzzer.runTest();\n"
       "}\n"
       "#ifdef __cplusplus\n"
       "}\n"
       "#endif\n"},
  };

  std::map<std::string, std::string> NewAnswers = {
      {UTModify::HeaderName, HeaderContent}};

  UTModify Modifier(*F, Loader->getSourceReport());
  ASSERT_TRUE(checkMod(ModAnswers, Modifier.getReplacements()));
  ASSERT_TRUE(checkNew(NewAnswers, Modifier.getNewFiles()));
}

TEST_F(TestUTModify, NoInputN) {
  SourceFileManager SFM;
  ASSERT_TRUE(SFM.createFile("library.h", "extern \"C\" {\n"
                                          "void API_1(int);\n"
                                          "}"));
  ASSERT_TRUE(SFM.createFile("library.cpp", "#include \"library.h\"\n"
                                            "extern \"C\" {\n"
                                            "void EXTERN_1(int);\n"
                                            "void API_1(int P) {\n"
                                            "  EXTERN_1(P);\n"
                                            "}\n"
                                            "}"));
  ASSERT_TRUE(SFM.createFile("ut.cpp", "#include \"library.h\"\n"
                                       "#include \"gtest/gtest.h\"\n"
                                       "TEST(Test, Test) {\n"
                                       "}\n"));
  auto Loader = createLoader(SFM, {"API_1"}, "library.cpp", {"ut.cpp"});
  ASSERT_TRUE(Loader);

  auto F = createFuzzer(*Loader);
  ASSERT_TRUE(F);

  const std::vector<Replacement> ModAnswers = {
      {"ut.cpp", 67, 0,
       "#ifdef __cplusplus\n"
       "extern \"C\" {\n"
       "#endif\n"
       "void enterAutofuzz() {\n"
       "  class AutofuzzTest : public Test_Test_Test {\n"
       "  public:\n"
       "    void runTest() {\n"
       "      try {\n"
       "        SetUpTestCase();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        SetUp();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TestBody();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TearDown();\n"
       "      } catch (std::exception &E) {}\n"
       "      try {\n"
       "        TearDownTestCase();\n"
       "      } catch (std::exception &E) {}\n"
       "    }\n"
       "  };\n"
       "  AutofuzzTest Fuzzer;\n"
       "  Fuzzer.runTest();\n"
       "}\n"
       "#ifdef __cplusplus\n"
       "}\n"
       "#endif\n"}};
  std::map<std::string, std::string> NewAnswers = {
      {UTModify::HeaderName, HeaderContent}};

  UTModify Modifier(*F, Loader->getSourceReport());
  ASSERT_TRUE(checkMod(ModAnswers, Modifier.getReplacements()));
  ASSERT_TRUE(checkNew(NewAnswers, Modifier.getNewFiles()));
}
