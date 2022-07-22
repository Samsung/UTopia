#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/inputfilter/CompileConstantFilter.h"
#include "ftg/inputfilter/ConstIntArrayLenFilter.h"
#include "ftg/inputfilter/ExternalFilter.h"
#include "ftg/inputfilter/InaccessibleGlobalFilter.h"
#include "ftg/inputfilter/InvalidLocationFilter.h"
#include "ftg/inputfilter/RawStringFilter.h"
#include "ftg/inputfilter/TypeUnavailableFilter.h"
#include "ftg/inputfilter/UnsupportTypeFilter.h"
#include "testutil/SourceFileManager.h"

namespace ftg {

class TestInputFilter : public testing::Test {
protected:
  std::unique_ptr<SourceCollection> SC;
  std::unique_ptr<IRAccessHelper> IRAccess;
  std::unique_ptr<ASTIRMap> ASTMap;

  bool load(const SourceFileManager &SFM, std::string Name) {
    auto Path = SFM.getFilePath(Name);
    if (Path.empty())
      return false;
    std::vector<std::string> SrcPaths = {Path};
    auto CH = TestHelperFactory().createCompileHelper(
        SFM.getBaseDirPath(), SrcPaths, "-O0 -g",
        CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    if (!SC)
      return false;

    IRAccess = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!IRAccess)
      return false;

    ASTMap = std::make_unique<DebugInfoMap>(*SC);
    if (!ASTMap)
      return false;

    return true;
  }

  bool load(std::string Code) {
    auto CH = TestHelperFactory().createCompileHelper(
        Code, "rawstringFilter", "-O0 -g", CompileHelper::SourceType_CPP);
    if (!CH)
      return false;

    SC = CH->load();
    if (!SC)
      return false;

    IRAccess = std::make_unique<IRAccessHelper>(SC->getLLVMModule());
    if (!IRAccess)
      return false;

    ASTMap = std::make_unique<DebugInfoMap>(*SC);
    if (!ASTMap)
      return false;

    return true;
  }

  std::shared_ptr<RDNode> generateRDNode(std::string FuncName, unsigned BIdx,
                                         unsigned IIdx, int OIdx) {
    auto *I = IRAccess->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return nullptr;

    if (OIdx >= (int)I->getNumOperands() || OIdx < -1)
      return nullptr;

    llvm::Value *V = I;
    if (OIdx > -1)
      V = I->getOperand(OIdx);

    auto Node = std::make_shared<RDNode>(*V, *I);
    auto *CB = llvm::dyn_cast<llvm::CallBase>(&Node->getLocation());
    if (CB && Node->getIdx() >= 0)
      Node->setFirstUse(*CB, Node->getIdx());
    return Node;
  }

  std::shared_ptr<RDNode> generateRDNode(std::string GlobalName,
                                         std::string FuncName, unsigned BIdx,
                                         unsigned IIdx) {
    auto *G = IRAccess->getGlobalVariable(GlobalName);
    auto *I = IRAccess->getInstruction(FuncName, BIdx, IIdx);
    if (!G || !I)
      return nullptr;

    return std::make_shared<RDNode>(*G, *I);
  }

  std::shared_ptr<ASTIRNode> generateASTIRNode(std::string FuncName,
                                               unsigned BIdx, unsigned IIdx,
                                               int OIdx) {
    auto Node = generateRDNode(FuncName, BIdx, IIdx, OIdx);
    if (!Node)
      return nullptr;

    auto D = Node->getDefinition();
    if (!D.first)
      return nullptr;

    auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
    if (!ADN)
      return nullptr;

    return std::make_shared<ASTIRNode>(*ADN, *Node);
  }

  std::shared_ptr<ASTIRNode> generateASTIRNode(std::string GlobalName,
                                               std::string FuncName,
                                               unsigned BIdx, unsigned IIdx) {
    auto Node = generateRDNode(GlobalName, FuncName, BIdx, IIdx);
    if (!Node)
      return nullptr;

    auto D = Node->getDefinition();
    if (!D.first)
      return nullptr;

    auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
    if (!ADN)
      return nullptr;

    return std::make_shared<ASTIRNode>(*ADN, *Node);
  }

  const llvm::Argument *argument(std::string FuncName, unsigned AIdx) {
    const auto *F = SC->getLLVMModule().getFunction(FuncName);
    if (!F)
      return nullptr;
    return F->getArg(AIdx);
  }

  bool check(std::string FuncName, unsigned BIdx, unsigned IIdx, int OIdx,
             const InputFilter &Filter) {
    auto Node = generateASTIRNode(FuncName, BIdx, IIdx, OIdx);
    return Node && Filter.start(*Node);
  }

  bool check(std::string GlobalName, std::string FuncName, unsigned BIdx,
             unsigned IIdx, const InputFilter &Filter) {
    auto Node = generateASTIRNode(GlobalName, FuncName, BIdx, IIdx);
    return Node && Filter.start(*Node);
  }

  bool checkNone(std::string FuncName, unsigned BIdx, unsigned IIdx, int OIdx,
                 const InputFilter &Filter) {
    auto Node = generateASTIRNode(FuncName, BIdx, IIdx, OIdx);
    if (!Node)
      return false;
    return !Filter.start(*Node);
  }

  bool checkNone(std::string GlobalName, std::string FuncName, unsigned BIdx,
                 unsigned IIdx, const InputFilter &Filter) {
    auto Node = generateASTIRNode(GlobalName, FuncName, BIdx, IIdx);
    if (!Node)
      return false;
    return !Filter.start(*Node);
  }
};

TEST_F(TestInputFilter, RawStringFilterP) {
  const std::string Code = "#include <string>\n"
                           "extern \"C\" {\n"
                           "void test() { std::string Var1 = R\"(Hello)\"; }\n"
                           "}";
  ASSERT_TRUE(load(Code));

  auto Node = generateASTIRNode("test", 0, 6, 1);
  ASSERT_TRUE(Node);

  RawStringFilter Filter;
  ASSERT_TRUE(Filter.start(*Node));
}

TEST_F(TestInputFilter, RawStringFilterN) {
  const std::string Code = "#include <string>\n"
                           "extern \"C\" {\n"
                           "void test() { std::string Var1 = \"Hello\"; }\n"
                           "}";
  ASSERT_TRUE(load(Code));

  auto Node = generateASTIRNode("test", 0, 6, 1);
  ASSERT_TRUE(Node);

  RawStringFilter Filter;
  ASSERT_FALSE(Filter.start(*Node));
}

TEST_F(TestInputFilter, ExternalFilterP) {
  const std::string Code =
      "extern \"C\" {\n"
      "struct ST { int F1; int F2; int F3; int F4; int F5; };\n"
      "class CLS { public: void M(); };\n"
      "int API_1();\n"
      "void API_2(int P);\n"
      "void API_3(int *P);\n"
      "ST API_4();\n"
      "void API_5(int, ...);\n"
      "void test_method_call() {\n"
      "  CLS().M();\n"
      "}\n"
      "void test_return() {\n"
      "  API_2(API_1());\n"
      "}\n"
      "void test_structret() {\n"
      "  ST Var = API_4();\n"
      "}\n"
      "void test_outparam() {\n"
      "  int L;\n"
      "  API_3(&L);\n"
      "}\n"
      "void test_vararg() {\n"
      "  API_5(0, 1, 2);\n"
      "}\n"
      "}";

  ASSERT_TRUE(load(Code));

  DirectionAnalysisReport Report;
  auto *Arg = argument("API_3", 0);
  ASSERT_TRUE(Arg);

  Report.set(*Arg, Dir_Out);

  Arg = argument("API_5", 0);
  ASSERT_TRUE(Arg);

  Report.set(*Arg, Dir_Out);

  ExternalFilter Filter(Report);
  ASSERT_TRUE(check("test_method_call", 0, 1, 0, Filter));
  ASSERT_TRUE(check("test_return", 0, 1, 0, Filter));
  ASSERT_TRUE(check("test_structret", 0, 2, 0, Filter));
  ASSERT_TRUE(check("test_vararg", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, ExternalFilterN) {
  const std::string Code = "extern \"C\" {\n"
                           "class CLS { public: void M(int); };\n"
                           "void API_1(int);\n"
                           "void API_2(int, ...);\n"
                           "void test_method_call() {\n"
                           "  CLS().M(10);\n"
                           "}\n"
                           "void test_func() {\n"
                           "  API_1(10);\n"
                           "}\n"
                           "void test_memset() {\n"
                           "  int Var[] = { 0, 1, 2 };\n"
                           "}\n"
                           "void test_vararg() {\n"
                           "  API_2(0, 1, 2);\n"
                           "}\n"
                           "}\n";

  ASSERT_TRUE(load(Code));

  DirectionAnalysisReport Report;
  ExternalFilter Filter(Report);
  ASSERT_TRUE(checkNone("test_method_call", 0, 1, 1, Filter));
  ASSERT_TRUE(checkNone("test_func", 0, 0, 0, Filter));
  ASSERT_TRUE(checkNone("test_memset", 0, 3, -1, Filter));
  ASSERT_TRUE(checkNone("test_vararg", 0, 0, 1, Filter));
  ASSERT_TRUE(checkNone("test_vararg", 0, 0, 2, Filter));
}

TEST_F(TestInputFilter, AvailableTypeFilterP) {
  const std::string Code = "extern \"C\" {\n"
                           "typedef unsigned uint32;\n"
                           "void API_1(int P);\n"
                           "struct ST1 { int F1; };\n"
                           "void test_defined_struct() {\n"
                           "  struct ST1 Var1 = { 10 };\n"
                           "  API_1(Var1.F1);\n"
                           "}\n"
                           "void test_typedef() {\n"
                           "  uint32 Var = 10;\n"
                           "  API_1(Var);\n"
                           "}\n"
                           "}";
  ASSERT_TRUE(load(Code));

  TypeUnavailableFilter Filter;
  ASSERT_TRUE(check("test_defined_struct", 0, 3, -1, Filter));
  ASSERT_TRUE(check("test_typedef", 0, 2, -1, Filter));
}

TEST_F(TestInputFilter, AvailableTypeFilterN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API_1(int);\n"
                           "void test_input() {\n"
                           "  API_1(10);\n"
                           "}\n"
                           "}\n";
  ASSERT_TRUE(load(Code));

  TypeUnavailableFilter Filter;
  ASSERT_TRUE(checkNone("test_input", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, CompileConstantP) {
  const std::string Code = "extern \"C\" {\n"
                           "#define MACRO_1(Var) sizeof(Var) / sizeof(Var[0])\n"
                           "void API_1(int);\n"
                           "void test_macro_const() {\n"
                           "  int Var1[] = { 0, 1, 2 };\n"
                           "  API_1(MACRO_1(Var1));\n"
                           "}\n"
                           "}\n";
  ASSERT_TRUE(load(Code));

  CompileConstantFilter Filter;
  ASSERT_TRUE(check("test_macro_const", 0, 4, 0, Filter));
}

TEST_F(TestInputFilter, CompileConstantN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API_1(int);\n"
                           "void test_input() {\n"
                           "  API_1(10);\n"
                           "}\n"
                           "}\n";
  ASSERT_TRUE(load(Code));

  CompileConstantFilter Filter;
  ASSERT_TRUE(checkNone("test_input", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, ConstIntArrayLenP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API_1(int);\n"
                           "void API_2(int*);\n"
                           "void test_direct() {\n"
                           "  const int Var = 10;\n"
                           "  int Var2[Var] = { 0, };\n"
                           "  API_2(&Var2[0]);\n"
                           "}\n"
                           "void test_operation() {\n"
                           "  const int Var = 10;\n"
                           "  int Var2[Var + 10] = { 0, };\n"
                           "  API_2(&Var2[0]);\n"
                           "}\n"
                           "}\n";
  ASSERT_TRUE(load(Code));

  ConstIntArrayLenFilter Filter;
  ASSERT_TRUE(check("test_direct", 0, 3, -1, Filter));
  ASSERT_TRUE(check("test_operation", 0, 3, -1, Filter));
}

TEST_F(TestInputFilter, ConstIntArrayLenN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API_1(int);\n"
                           "void test_input() {\n"
                           "  const int Var = 10;\n"
                           "  API_1(Var);\n"
                           "}\n"
                           "}\n";
  ASSERT_TRUE(load(Code));

  ConstIntArrayLenFilter Filter;
  ASSERT_TRUE(checkNone("test_input", 0, 2, -1, Filter));
}

TEST_F(TestInputFilter, InaccessibleGlobalP) {
  const std::string Code = "extern \"C\" {\n"
                           "class CLS {\n"
                           "public:\n"
                           "  int F1 = 1;\n"
                           "  CLS() = default;\n"
                           "private:\n"
                           "  int F2 = 2;\n"
                           "  static int F3;\n"
                           "protected:\n"
                           "  static int F4;\n"
                           "};\n"
                           "int CLS::F3 = 3, CLS::F4 = 4;\n"
                           "void test() {\n"
                           "  CLS Var1;\n"
                           "}\n"
                           "}";
  ASSERT_TRUE(load(Code));

  InaccessibleGlobalFilter Filter;
  ASSERT_TRUE(check("_ZN3CLSC2Ev", 0, 5, -1, Filter));
  ASSERT_TRUE(check("_ZN3CLSC2Ev", 0, 7, -1, Filter));
  ASSERT_TRUE(check("_ZN3CLS2F3E", "test", 0, 0, Filter));
  ASSERT_TRUE(check("_ZN3CLS2F4E", "test", 0, 0, Filter));
}

TEST_F(TestInputFilter, InaccessibleGlobalN) {
  const std::string Code = "extern \"C\" {\n"
                           "class CLS {\n"
                           "public:\n"
                           "  static int F1;\n"
                           "  CLS() : F2(1) { F3 = 2; };\n"
                           "  void set() { F4 = 1000; };\n"
                           "private:\n"
                           "  int F2;\n"
                           "  int F3;\n"
                           "  static int F4;\n"
                           "};\n"
                           "int CLS::F1 = 10;\n"
                           "void test() { CLS Var1; Var1.set(); }\n"
                           "}\n";
  ASSERT_TRUE(load(Code));
  llvm::outs() << SC->getLLVMModule() << "\n";

  InaccessibleGlobalFilter Filter;
  ASSERT_TRUE(checkNone("_ZN3CLS2F1E", "test", 0, 0, Filter));
  ASSERT_TRUE(checkNone("_ZN3CLSC2Ev", 0, 5, -1, Filter));
  ASSERT_TRUE(checkNone("_ZN3CLSC2Ev", 0, 7, -1, Filter));
  ASSERT_TRUE(checkNone("_ZN3CLS3setEv", 0, 4, -1, Filter));
}

TEST_F(TestInputFilter, InvalidLocationP) {
  const std::string HeaderCode = "extern \"C\" {\n"
                                 "int sub() {\n"
                                 "  return 10;\n"
                                 "}\n"
                                 "}\n";
  const std::string SourceCode = "#include \"header.h\"\n"
                                 "extern \"C\" {\n"
                                 "void API(int);\n"
                                 "void test() {\n"
                                 "  API(sub());\n"
                                 "}\n"
                                 "}\n";

  SourceFileManager SFM;
  SFM.createFile("header.h", HeaderCode);
  SFM.createFile("source.cpp", SourceCode);
  ASSERT_TRUE(load(SFM, "source.cpp"));

  InvalidLocationFilter Filter(SFM.getBaseDirPath());
  ASSERT_TRUE(check("sub", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, InvalidLocationN) {
  const std::string SourceCode = "extern \"C\" {\n"
                                 "void API(int);\n"
                                 "int sub() {\n"
                                 "  return 10;\n"
                                 "}\n"
                                 "void test() {\n"
                                 "  API(sub());\n"
                                 "}\n"
                                 "}\n";

  SourceFileManager SFM;
  SFM.createFile("source.cpp", SourceCode);
  ASSERT_TRUE(load(SFM, "source.cpp"));

  InvalidLocationFilter Filter(SFM.getBaseDirPath());
  ASSERT_TRUE(checkNone("sub", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, UnsupportTypeP) {
  const std::string SourceCode =
      "extern \"C\" {\n"
      "namespace {\n"
      "enum E1 { E11, E12 };\n"
      "}\n"
      "struct ST1 { int F; };\n"
      "struct ST2 { const int F; };\n"
      "struct ST3 { int F1[10][10]; struct ST3 *F2; };\n"
      "class CLS {\n"
      "  public: CLS() : F(10) {}\n"
      "  private: int F; };\n"
      "void API(const void *);\n"
      "void pointer() {\n"
      "  int *Var;\n"
      "}\n"
      "void variable_length_array(int P) {\n"
      "  int Var[P];\n"
      "}\n"
      "void multidimensional_array() {\n"
      "  int Var[10][10];\n"
      "}\n"
      "void pointer_array() {\n"
      "  int *Var[10];\n"
      "}\n"
      "void structure_array() {\n"
      "  ST1 Var[10];\n"
      "}\n"
      "void realtype_class() {\n"
      "  CLS Var;\n"
      "}\n"
      "void struct_const_field() {\n"
      "  ST2 Var = { .F = 10 };\n"
      "}\n"
      "void struct_unsupport() {\n"
      "  ST3 Var;\n"
      "}\n"
      "void anonymous_enum() {\n"
      "  E1 Var;\n"
      "}\n"
      "void void_pointer() {\n"
      "  API(nullptr);\n"
      "}\n"
      "}";
  SourceFileManager SFM;
  SFM.createFile("source.cpp", SourceCode);
  ASSERT_TRUE(load(SFM, "source.cpp"));

  UnsupportTypeFilter Filter;
  ASSERT_TRUE(check("pointer", 0, 0, -1, Filter));
  if (getClangVersion() == "10.0.0") {
    ASSERT_TRUE(check("variable_length_array", 0, 8, -1, Filter));
  } else {
    ASSERT_TRUE(check("variable_length_array", 0, 9, -1, Filter));
  }
  ASSERT_TRUE(check("multidimensional_array", 0, 0, -1, Filter));
  ASSERT_TRUE(check("pointer_array", 0, 0, -1, Filter));
  ASSERT_TRUE(check("structure_array", 0, 0, -1, Filter));
  ASSERT_TRUE(check("realtype_class", 0, 0, -1, Filter));
  ASSERT_TRUE(check("struct_const_field", 0, 0, -1, Filter));
  ASSERT_TRUE(check("struct_unsupport", 0, 0, -1, Filter));
  ASSERT_TRUE(check("anonymous_enum", 0, 0, -1, Filter));
  ASSERT_TRUE(check("void_pointer", 0, 0, 0, Filter));
}

TEST_F(TestInputFilter, UnsupportTypeN) {
  const std::string SourceCode = "#include <string>\n"
                                 "struct ST1 { int F; };\n"
                                 "extern \"C\" {\n"
                                 "void API(const void *);\n"
                                 "void integer_types() {\n"
                                 "  char Var1;\n"
                                 "  unsigned char Var2;\n"
                                 "  short Var3;\n"
                                 "  unsigned short Var4;\n"
                                 "  int Var5;\n"
                                 "  unsigned int Var6;\n"
                                 "  long Var7;\n"
                                 "  unsigned long Var8;\n"
                                 "  long long Var9;\n"
                                 "  unsigned long long Var10;\n"
                                 "}\n"
                                 "void float_types() {\n"
                                 "  float Var1;\n"
                                 "  double Var2;\n"
                                 "  long double Var3;\n"
                                 "}\n"
                                 "void string_types() {\n"
                                 "  char *Var1;\n"
                                 "  unsigned char *Var2;\n"
                                 "  char Var3[10];\n"
                                 "  unsigned char Var4[10];\n"
                                 "}\n"
                                 "void array_types() {\n"
                                 "  int Var1[10];\n"
                                 "  float Var2[10];\n"
                                 "}\n"
                                 "}";
  SourceFileManager SFM;
  SFM.createFile("source.cpp", SourceCode);
  ASSERT_TRUE(load(SFM, "source.cpp"));

  UnsupportTypeFilter Filter;
  ASSERT_TRUE(checkNone("integer_types", 0, 0, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 1, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 2, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 3, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 4, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 5, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 6, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 7, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 8, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 9, -1, Filter));
  ASSERT_TRUE(checkNone("integer_types", 0, 10, -1, Filter));
  ASSERT_TRUE(checkNone("float_types", 0, 0, -1, Filter));
  ASSERT_TRUE(checkNone("float_types", 0, 1, -1, Filter));
  ASSERT_TRUE(checkNone("float_types", 0, 2, -1, Filter));
  ASSERT_TRUE(checkNone("string_types", 0, 0, -1, Filter));
  ASSERT_TRUE(checkNone("string_types", 0, 1, -1, Filter));
  ASSERT_TRUE(checkNone("string_types", 0, 2, -1, Filter));
  ASSERT_TRUE(checkNone("string_types", 0, 3, -1, Filter));
  ASSERT_TRUE(checkNone("array_types", 0, 0, -1, Filter));
  ASSERT_TRUE(checkNone("array_types", 0, 1, -1, Filter));
}

} // namespace ftg
