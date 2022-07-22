#include "TestInputAnalysisBase.hpp"
#include "testutil/SourceFileManager.h"

class TestDefMapGenerator : public TestInputAnalysisBase {
protected:
  std::shared_ptr<IRAccessHelper> IRAccess;
  std::unique_ptr<ASTIRMap> ASTMap;

  std::shared_ptr<ASTIRNode> generateASTIRNode(const RDNode &Node) {
    std::vector<RDNode> Nodes = {Node};
    auto ASTIRNodes = generateASTIRNodes(Nodes);
    if (ASTIRNodes.size() != 1)
      return nullptr;
    return std::make_shared<ASTIRNode>(ASTIRNodes[0]);
  }

  std::vector<ASTIRNode> generateASTIRNodes(const std::vector<RDNode> &Nodes) {
    if (!ASTMap)
      return {};

    std::vector<ASTIRNode> Result;
    for (const auto &Node : Nodes) {
      auto D = Node.getDefinition();
      if (!D.first)
        return {};

      auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
      if (!ADN)
        return {};

      Result.emplace_back(*ADN, Node);
    }
    return Result;
  }

  std::shared_ptr<Definition> generateDefinition(std::string FuncName,
                                                 unsigned BIdx, unsigned IIdx,
                                                 int OIdx) {
    auto Node = generateRDNode(FuncName, BIdx, IIdx, OIdx);
    if (!Node)
      return nullptr;

    std::vector<RDNode> Nodes = {*Node};
    auto Defs = generateDefinitions(Nodes);
    if (Defs.size() != 1)
      return nullptr;
    return std::make_shared<Definition>(Defs[0]);
  }

  std::vector<Definition>
  generateDefinitions(const std::vector<RDNode> &Nodes) {
    auto DG = getDefMapGenerator(Nodes);

    std::vector<Definition> Defs;
    for (const auto &Iter : DG.getDefMap()) {
      if (!Iter.second)
        return {};
      Defs.emplace_back(*Iter.second);
    }

    return Defs;
  }

  const std::shared_ptr<Definition> generateDefinition(std::string GlobalName,
                                                       std::string FuncName,
                                                       unsigned BIdx,
                                                       unsigned IIdx) {
    auto Node = generateRDNode(GlobalName, FuncName, BIdx, IIdx);
    if (!Node)
      return nullptr;

    std::vector<RDNode> RDNodes = {*Node};
    auto DG = getDefMapGenerator(RDNodes);
    const auto &DM = DG.getDefMap();
    if (DM.size() != 1)
      return nullptr;

    auto Iter = DM.find(0);
    if (Iter == DM.end())
      return nullptr;

    return Iter->second;
  }

  DefMapGenerator getDefMapGenerator(const std::vector<RDNode> &RDNodes) {
    DefMapGenerator DG;
    if (!IRAccess || !Loader)
      return DG;

    std::vector<ASTIRNode> ASTIRNodes;
    for (const auto &Node : RDNodes) {
      auto D = Node.getDefinition();
      if (!D.first)
        continue;

      auto *ADN = ASTMap->getASTDefNode(*D.first, D.second);
      if (!ADN)
        continue;

      ASTIRNodes.emplace_back(*ADN, Node);
    }

    DG.generate(ASTIRNodes, *Loader);
    return DG;
  }

  std::shared_ptr<RDNode> generateRDNode(std::string FuncName, unsigned BIdx,
                                         unsigned IIdx, int OIdx) {
    std::vector<InstIndex> Srcs = {{FuncName, BIdx, IIdx, OIdx}};
    auto Nodes = generateRDNodes(Srcs);
    if (Nodes.size() != 1)
      return nullptr;
    return std::make_shared<RDNode>(Nodes[0]);
  }

  std::vector<RDNode> generateRDNodes(const std::vector<InstIndex> &Srcs) {
    if (!IRAccess)
      return {};

    std::vector<RDNode> Nodes;
    for (const auto &Src : Srcs) {
      auto *I = IRAccess->getInstruction(Src.FuncName, Src.BIdx, Src.IIdx);
      if (!I)
        return {};
      if (Src.OIdx >= (int)I->getNumOperands() || Src.OIdx < -1)
        return {};

      llvm::Value *V = I;
      if (Src.OIdx > -1)
        V = I->getOperand(Src.OIdx);
      RDNode Node(*V, *I);
      auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(&Node.getLocation());
      if (CB && Node.getIdx() >= 0)
        Node.setFirstUse(*CB, Node.getIdx());
      Nodes.emplace_back(Node);
    }
    return Nodes;
  }

  std::shared_ptr<RDNode> generateRDNode(std::string GlobalName,
                                         std::string FuncName, unsigned BIdx,
                                         unsigned IIdx) {
    auto *G = IRAccess->getGlobalVariable(GlobalName);
    if (!G)
      return nullptr;

    auto *I = IRAccess->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return nullptr;

    auto Node = std::make_shared<RDNode>(*G, *I);
    if (!Node)
      return nullptr;

    Node->setIdx(-1);
    return Node;
  }

  bool init(std::string SrcDir, std::string CodePath) {
    std::vector<std::string> CodePaths = { CodePath };
    return init(SrcDir, CodePaths);
  }

  bool init(std::string SrcDir, std::vector<std::string> CodePaths) {
    if (!load(SrcDir, CodePaths))
      return false;

    const auto &SC = Loader->getSourceCollection();
    IRAccess = std::make_shared<IRAccessHelper>(SC.getLLVMModule());
    if (!IRAccess)
      return false;

    ASTMap = std::make_unique<DebugInfoMap>(SC);
    if (!ASTMap)
      return false;

    return true;
  }
};

TEST_F(TestDefMapGenerator, generate_ArrayGroupP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API1(int *, int);\n"
                           "int API2();\n"
                           "void test() {\n"
                           "  int Var1[128] = { 0, };\n"
                           "  int Var2[128] = { 0, };\n"
                           "  int Var3[128] = { 0, };\n"
                           "  int Var4[128] = { 0, };\n"
                           "  int Var5 = API2();\n"
                           "  int Var6 = API2();\n"
                           "  API1(Var1, Var5);\n"
                           "  API1(Var2, Var5);\n"
                           "  API1(Var3, Var6);\n"
                           "  API1(Var4, Var6);\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  ArrayAnalysisReport ArrayReport;
  ArrayReport.set("API1", 0, 1);
  Loader->setArrayReport(ArrayReport);

  auto Node1 = generateRDNode("test", 0, 8, -1);
  auto Node2 = generateRDNode("test", 0, 11, -1);
  auto Node3 = generateRDNode("test", 0, 14, -1);
  auto Node4 = generateRDNode("test", 0, 17, -1);
  auto Node5 = generateRDNode("test", 0, 19, -1);
  auto Node6 = generateRDNode("test", 0, 22, -1);
  ASSERT_TRUE(Node1 && Node2 && Node3 && Node4 && Node5 && Node6);

  auto *API1_1 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 26));
  auto *API1_2 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 29));
  auto *API1_3 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 32));
  auto *API1_4 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 35));
  ASSERT_TRUE(API1_1 && API1_2 && API1_3 && API1_4);

  Node1->setFirstUse(*API1_1, 0);
  Node2->setFirstUse(*API1_2, 0);
  Node3->setFirstUse(*API1_3, 0);
  Node4->setFirstUse(*API1_4, 0);
  Node5->setFirstUse(*API1_1, 1);
  Node5->addFirstUse(*API1_2, 1);
  Node6->setFirstUse(*API1_3, 1);
  Node6->addFirstUse(*API1_4, 1);

  std::vector<RDNode> Nodes = {*Node1, *Node2, *Node3, *Node4, *Node5, *Node6};
  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 6);

  for (const auto &Iter : Defs) {
    ASSERT_NE(Iter.Filters.find(ArrayGroupFilter::FilterName),
              Iter.Filters.end());
  }
}

TEST_F(TestDefMapGenerator, generate_ArrayGroupN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API1(int *, int);\n"
                           "int API2();\n"
                           "void test() {\n"
                           "  int Var1[128] = { 0, };\n"
                           "  int Var2[128] = { 0, };\n"
                           "  int Var3[128] = { 0, };\n"
                           "  int Var4[128] = { 0, };\n"
                           "  int Var5 = API2();\n"
                           "  int Var6 = API2();\n"
                           "  API1(Var1, Var5);\n"
                           "  API1(Var2, Var5);\n"
                           "  API1(Var3, Var6);\n"
                           "  API1(Var4, Var6);\n"
                           "}}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  auto Node1 = generateRDNode("test", 0, 8, -1);
  auto Node2 = generateRDNode("test", 0, 11, -1);
  auto Node3 = generateRDNode("test", 0, 14, -1);
  auto Node4 = generateRDNode("test", 0, 17, -1);
  auto Node5 = generateRDNode("test", 0, 19, -1);
  auto Node6 = generateRDNode("test", 0, 22, -1);
  ASSERT_TRUE(Node1 && Node2 && Node3 && Node4 && Node5 && Node6);

  auto *API1_1 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 26));
  auto *API1_2 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 29));
  auto *API1_3 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 32));
  auto *API1_4 = llvm::dyn_cast_or_null<llvm::CallBase>(
      IRAccess->getInstruction("test", 0, 35));
  ASSERT_TRUE(API1_1 && API1_2 && API1_3 && API1_4);

  Node1->setFirstUse(*API1_1, 0);
  Node2->setFirstUse(*API1_2, 0);
  Node3->setFirstUse(*API1_3, 0);
  Node4->setFirstUse(*API1_4, 0);
  Node5->setFirstUse(*API1_1, 1);
  Node5->addFirstUse(*API1_2, 1);
  Node6->setFirstUse(*API1_3, 1);
  Node6->addFirstUse(*API1_4, 1);

  std::vector<RDNode> Nodes = {*Node1, *Node2, *Node3, *Node4, *Node5, *Node6};
  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 6);

  for (const auto &Iter : Defs) {
    ASSERT_EQ(Iter.Filters.find(ArrayGroupFilter::FilterName),
              Iter.Filters.end());
  }
}

TEST_F(TestDefMapGenerator, generate_AssignOperatorRequiredP) {
  const std::string Code =
      "extern \"C\" {\n"
      "struct ST { int F1; int F2; };\n"
      "void test_basic() { int A; }\n"
      "void test_st_1() { struct ST V; }\n"
      "void test_st_2() { struct ST V { .F1 = 10, .F2 = 20 }; }\n"
      "void test_st_3() { struct ST V{10, 20}; }\n"
      "void test_array() { int A[]{1, 2, 3}; }\n"
      "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::vector<InstIndex> SrcInsts = {{"test_basic", 0, 0, -1},
                                     {"test_st_1", 0, 0, -1},
                                     {"test_st_2", 0, 3, -1},
                                     {"test_st_3", 0, 3, -1},
                                     {"test_array", 0, 3, -1}};
  auto Nodes = generateRDNodes(SrcInsts);
  ASSERT_EQ(Nodes.size(), 5);

  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 5);
  for (const auto &Def : Defs)
    ASSERT_TRUE(Def.AssignOperatorRequired);
}

TEST_F(TestDefMapGenerator, generate_AssignOperatorRequiredN) {
  const std::string Code = "extern \"C\" {\n"
                           "struct ST { int F1; int F2; };\n"
                           "void test_basic() { int A = 10; }\n"
                           "void test_st() { struct ST V = { 10, 20 }; }\n"
                           "void test_array() { int A[] = { 1, 2, 3 }; }\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::vector<InstIndex> SrcInsts = {
      {"test_basic", 0, 2, -1},
      {"test_st", 0, 3, -1},
  };
  auto Nodes = generateRDNodes(SrcInsts);
  ASSERT_EQ(Nodes.size(), 2);

  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 2);
  for (const auto &Def : Defs)
    ASSERT_FALSE(Def.AssignOperatorRequired);
}

TEST_F(TestDefMapGenerator, generate_DeclP) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int);\n"
                           "int GVar1;\n"
                           "int dummy_global() {\n"
                           "  return 1;\n"
                           "}\n"
                           "void test_decl_static_local() {\n"
                           "  static int Var = 1;\n"
                           "  API(Var);\n"
                           "}\n"
                           "void test_decl_local() {\n"
                           "  int Var = 1;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  // int GVar;
  D = generateDefinition("GVar1", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Declaration, Definition::DeclType_Global);
  ASSERT_EQ(D->TypeOffset, 28);
  ASSERT_EQ(D->TypeString, "int ");
  ASSERT_EQ(D->EndOffset, 186);
  ASSERT_EQ(D->VarName, "GVar1");

  // static int Var = 1;
  D = generateDefinition("_ZZ22test_decl_static_localE3Var",
                         "test_decl_static_local", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Declaration, Definition::DeclType_StaticLocal);
  ASSERT_EQ(D->TypeOffset, 108);
  ASSERT_EQ(D->TypeString, "static int ");
  ASSERT_EQ(D->EndOffset, 127);
  ASSERT_EQ(D->VarName, "Var");

  // int Var = 1;
  D = generateDefinition("test_decl_local", 0, 2, -1);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Declaration, Definition::DeclType_Decl);
  ASSERT_EQ(D->TypeOffset, 169);
  ASSERT_EQ(D->TypeString, "int ");
  ASSERT_EQ(D->EndOffset, 181);
  ASSERT_EQ(D->VarName, "Var");
}

TEST_F(TestDefMapGenerator, generate_DeclN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(int);\n"
                           "void test_decl_none() {\n"
                           "  int Var;\n"
                           "  Var = 1;\n"
                           "  API(2);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  // Var = 1;
  D = generateDefinition("test_decl_none", 0, 2, -1);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Declaration, Definition::DeclType_None);
  ASSERT_EQ(D->TypeOffset, 0);
  ASSERT_TRUE(D->TypeString.empty());
  ASSERT_EQ(D->EndOffset, 0);
  ASSERT_TRUE(D->VarName.empty());

  // API(2);
  D = generateDefinition("test_decl_none", 0, 3, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Declaration, Definition::DeclType_None);
  ASSERT_EQ(D->TypeOffset, 0);
  ASSERT_TRUE(D->TypeString.empty());
  ASSERT_EQ(D->EndOffset, 0);
  ASSERT_TRUE(D->VarName.empty());
}

TEST_F(TestDefMapGenerator, generate_FiltersP) {
  const std::string HeaderCode = "extern \"C\" {\n"
                                 "int test_header() {\n"
                                 "  return 10;\n"
                                 "}\n"
                                 "}\n";
  const std::string SourceCode =
      "extern \"C\" {\n"
      "#include \"header.h\"\n"
      "#define MACRO_1(Var) sizeof(Var) / sizeof(Var[0])\n"
      "struct ST { int F; };\n"
      "class CLS1 {\n"
      "private: static int F1;\n"
      "};\n"
      "int CLS1::F1 = 10;\n"
      "void API_1(const char *P);\n"
      "int API_2();\n"
      "void API_3(int);\n"
      "int dummy_global() {\n"
      "  return 1;\n"
      "}\n"
      "void test_nullptr() {\n"
      "  API_1(nullptr);\n"
      "}\n"
      "void test_rawstring() {\n"
      "  API_1(R\"(local foo = \"bar\";)\");\n"
      "}\n"
      "void test_external() {\n"
      "  API_3(API_2());\n"
      "}\n"

      "void test_typeunavailable() {\n"
      "  ST Var1 = { 10 };\n"
      "  API_3(Var1.F);\n"
      "}\n"

      "void test_compileconst() {\n"
      "  int Var[] = { 0, 1, 2 };\n"
      "  API_3(MACRO_1(Var));\n"
      "}\n"
      "void API_4(void *);\n"
      "void test_constintarraylen() {\n"
      "  const int Var1 = 10;\n"
      "  int Var2[Var1] = { 0,  };\n"
      "  API_4(&Var2[0]);\n"
      "}\n"

      "void test_unsupporttype() {\n"
      "  int Var1[10][10];\n"
      "}\n"
      "}";
  SourceFileManager SFM;
  SFM.createFile("header.h", HeaderCode);
  SFM.createFile("test.cpp", SourceCode);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  D = generateDefinition("test_compileconst", 0, 4, 0);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(CompileConstantFilter::FilterName),
            D->Filters.end());

  D = generateDefinition("test_constintarraylen", 0, 3, -1);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(ConstIntArrayLenFilter::FilterName),
            D->Filters.end());

  D = generateDefinition("test_external", 0, 0, -1);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(ExternalFilter::FilterName), D->Filters.end());

  D = generateDefinition("_ZN4CLS12F1E", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(InaccessibleGlobalFilter::FilterName),
            D->Filters.end());

  D = generateDefinition("test_header", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(InvalidLocationFilter::FilterName),
            D->Filters.end());

  D = generateDefinition("test_nullptr", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(NullPointerFilter::FilterName), D->Filters.end());

  D = generateDefinition("test_rawstring", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(RawStringFilter::FilterName), D->Filters.end());

  D = generateDefinition("test_typeunavailable", 0, 3, -1);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(TypeUnavailableFilter::FilterName),
            D->Filters.end());

  D = generateDefinition("test_unsupporttype", 0, 0, -1);
  ASSERT_TRUE(D);
  ASSERT_NE(D->Filters.find(UnsupportTypeFilter::FilterName), D->Filters.end());
}

TEST_F(TestDefMapGenerator, generate_FiltersN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API_1(int P);\n"
                           "void test_input() {\n"
                           "  API_1(0);\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  D = generateDefinition("test_input", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->Filters.empty());
}

TEST_F(TestDefMapGenerator, generate_LocationP) {
  const std::string Code = "extern \"C\" {\n"
                           "void test() {\n"
                           "  int Var1;\n"
                           "  int Var2 = 10;\n"
                           "}\n"
                           "}";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::vector<InstIndex> SrcInsts = {{"test", 0, 0, -1}, {"test", 0, 4, -1}};
  auto RNodes = generateRDNodes(SrcInsts);
  ASSERT_EQ(RNodes.size(), 2);

  auto Defs = generateDefinitions(RNodes);
  ASSERT_EQ(Defs.size(), 2);

  ASSERT_EQ(Defs[0].Path, SFM.getFilePath("test.cpp"));
  ASSERT_EQ(Defs[0].Offset, 37);
  ASSERT_EQ(Defs[0].Length, 0);
  ASSERT_EQ(Defs[1].Path, SFM.getFilePath("test.cpp"));
  ASSERT_EQ(Defs[1].Offset, 52);
  ASSERT_EQ(Defs[1].Length, 2);
}

TEST_F(TestDefMapGenerator, generate_NamespaceP) {
  const std::string Code = "extern \"C\" {\n"
                           "int dummy_global() {\n"
                           "  return 1;\n"
                           "}\n"
                           "namespace {\n"
                           "  int GVar1;\n"
                           "}\n"
                           "namespace N1 {\n"
                           "  int GVar2;\n"
                           "  namespace N2 {\n"
                           "    int GVar3;\n"
                           "  }\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  // GVar1;
  D = generateDefinition("GVar1", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Namespace, "::");

  // GVar2;
  D = generateDefinition("GVar2", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Namespace, "N1::");

  // GVar3;
  D = generateDefinition("GVar3", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Namespace, "N1::N2::");
}

TEST_F(TestDefMapGenerator, generate_NamespaceN) {
  const std::string Code = "extern \"C\" {\n"
                           "int GVar;\n"
                           "int dummy_global() {\n"
                           "  return 1;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;
  // GVar;
  D = generateDefinition("GVar", "dummy_global", 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->Namespace.empty());
}

TEST_F(TestDefMapGenerator, generate_PropertiesP) {
  const std::string Code = "#include <stdlib.h>\n"
                           "#include <string>\n"
                           "extern \"C\" {\n"
                           "void API_1(const char *, int);\n"
                           "void API_2(int);\n"
                           "void API_3(const char *);\n"
                           "void API_4(int);\n"
                           "void test_array() {\n"
                           "  API_1(\"Hello\", 5);\n"
                           "}\n"
                           "void test_buffersize() {\n"
                           "  int *Var1 = new int[10];\n"
                           "  int *Var2 = (int *)malloc(10);\n"
                           "  API_2(0);\n"
                           "  std::string message(\"abc\", 3);\n"
                           "}\n"
                           "void test_filepath() {\n"
                           "  API_3(\"Hello.txt\");\n"
                           "}\n"
                           "void test_loopexit() {\n"
                           "  API_4(10);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs) && Loader);

  AllocAnalysisReport AllocReport;
  AllocReport.set("API_2", 0, Alloc_Size);
  AllocReport.set("_ZN3CLS3M_1Ei", 1, Alloc_Size);
  AllocAnalyzer AAnalyzer(Loader->getSourceCollection().getLLVMModule(),
                          &AllocReport);
  Loader->setAllocReport(AAnalyzer.result());

  ArrayAnalysisReport ArrayReport;
  ArrayReport.set("API_1", 0, 1);
  Loader->setArrayReport(ArrayReport);

  FilePathAnalysisReport FilePathReport;
  FilePathReport.set("API_3", 0, true);
  Loader->setFilePathReport(FilePathReport);

  LoopAnalysisReport LoopReport;
  LoopReport.set("API_4", 0, {true, 1});
  Loader->setLoopReport(LoopReport);

  std::vector<Definition> Ds;
  std::shared_ptr<Definition> D;

  // API_1("Hello", 5)
  std::vector<InstIndex> SrcInsts = {{"test_array", 0, 0, 0},
                                     {"test_array", 0, 0, 1}};
  auto Nodes = generateRDNodes(SrcInsts);
  ASSERT_EQ(Nodes.size(), 2);

  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 2);
  ASSERT_TRUE(Defs[0].Array);
  ASSERT_TRUE(Defs[1].ArrayLen);
  ASSERT_NE(Defs[0].ArrayLenIDs.find(1), Defs[0].ArrayLenIDs.end());
  ASSERT_NE(Defs[1].ArrayIDs.find(0), Defs[1].ArrayIDs.end());

  // new int[10]
  D = generateDefinition("test_buffersize", 0, 7, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->BufferAllocSize);

  // malloc(10)
  D = generateDefinition("test_buffersize", 0, 11, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->BufferAllocSize);

  // API_1(10)
  D = generateDefinition("test_buffersize", 0, 14, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->BufferAllocSize);

  // std::string("abc", 3)
  D = generateDefinition("test_buffersize", 0, 17, 2);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->BufferAllocSize);

  // API_3("Hello.txt");
  D = generateDefinition("test_filepath", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->FilePath);

  // API_4(10)
  D = generateDefinition("test_loopexit", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->LoopExit);
}

TEST_F(TestDefMapGenerator, generate_PropertiesN) {
  const std::string Code = "extern \"C\" {\n"
                           "void API(const char*, unsigned);\n"
                           "void test() {\n"
                           "  int Var = 10;\n"
                           "  API(\"Hello\", 5);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs) && Loader);

  std::shared_ptr<Definition> D;
  D = generateDefinition("test", 0, 2, -1);
  ASSERT_TRUE(D);
  ASSERT_FALSE(D->BufferAllocSize);
  ASSERT_FALSE(D->FilePath);
  ASSERT_FALSE(D->LoopExit);

  std::vector<InstIndex> SrcInsts = {{"test", 0, 3, 0}, {"test", 0, 3, 1}};
  auto Nodes = generateRDNodes(SrcInsts);
  auto Defs = generateDefinitions(Nodes);
  ASSERT_EQ(Defs.size(), 2);
  ASSERT_FALSE(Defs[0].Array);
  ASSERT_FALSE(Defs[1].ArrayLen);
}

TEST_F(TestDefMapGenerator, generate_TypeP) {
  const std::string Code =
      R"(
      extern "C" {
      void API(const void *);
      void test_void() {
        API("Hello");
      }
      }
      )";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(init(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  auto D = generateDefinition("test_void", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->DataType);
  ASSERT_EQ(D->DataType->getASTTypeName(), "char *");
}

TEST_F(TestDefMapGenerator, generate_TypeN) {
  const std::string Code =
      R"(
      extern "C" {
      void API(const void *);
      void test_void() {
        API(nullptr);
      }
      }
      )";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  ASSERT_TRUE(init(SFM.getBaseDirPath(), SFM.getFilePath("test.cpp")));

  auto D = generateDefinition("test_void", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_TRUE(D->DataType);
  ASSERT_EQ(D->DataType->getASTTypeName(), "const void *");
}

TEST_F(TestDefMapGenerator, generate_ValueP) {
  const std::string Code = "extern \"C\" {\n"
                           "extern int GVar;\n"
                           "void API(int);\n"
                           "void test() {\n"
                           "  API(1);\n"
                           "  API(GVar);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  Json::Value Root;
  const std::string JsonStr = R"(
    {
      "array": false,
      "values": [
        {
          "type": 0,
          "value": "10"
        }
      ]
    })";
  std::stringstream Stream(JsonStr);
  Json::Value ASTValueJson;
  Stream >> ASTValueJson;
  ASTValue Value = ASTValue(ASTValueJson);

  ConstAnalyzerReport ConstReport;
  ASSERT_EQ(Value.getValues().size(), 1);
  ASSERT_EQ(Value.getValues()[0].Value, "10");
  ASSERT_EQ(Value.getValues()[0].Type, ASTValueData::VType_INT);

  ConstReport.addConst("GVar", Value);
  Loader->setConstReport(ConstReport);

  std::shared_ptr<Definition> D;

  D = generateDefinition("test", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Value.getValues().size(), 1);
  ASSERT_EQ(D->Value.getValues()[0].Type, ASTValueData::VType_INT);
  ASSERT_EQ(D->Value.getValues()[0].Value, "1");

  D = generateDefinition("test", 0, 2, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Value.getValues().size(), 1);
  ASSERT_EQ(D->Value.getValues()[0].Type, ASTValueData::VType_INT);
  ASSERT_EQ(D->Value.getValues()[0].Value, "10");
}

TEST_F(TestDefMapGenerator, generate_ValueN) {
  const std::string Code = "extern \"C\" {\n"
                           "extern int GVar;\n"
                           "void API(int);\n"
                           "void test() {\n"
                           "  API(GVar);\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  std::shared_ptr<Definition> D;

  D = generateDefinition("test", 0, 0, 0);
  ASSERT_TRUE(D);
  ASSERT_EQ(D->Value.getValues().size(), 0);
}

TEST_F(TestDefMapGenerator, getIDP) {
  const std::string Code = "extern \"C\" {\n"
                           "void test() {\n"
                           "  int Var1 = 1;\n"
                           "  int Var2 = 2;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> SrcPaths = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), SrcPaths));

  std::vector<InstIndex> Srcs = {{"test", 0, 3, -1}, {"test", 0, 5, -1}};
  auto Nodes = generateRDNodes(Srcs);
  auto DG = getDefMapGenerator(Nodes);
  auto ASTIRNodes = generateASTIRNodes(Nodes);
  ASSERT_EQ(ASTIRNodes.size(), 2);
  ASSERT_EQ(DG.getID(ASTIRNodes[0].AST), 0);
  ASSERT_EQ(DG.getID(ASTIRNodes[1].AST), 1);
}

TEST_F(TestDefMapGenerator, getIDN) {
  const std::string Code = "extern \"C\" {\n"
                           "void test() {\n"
                           "  int Var1 = 1;\n"
                           "  int Var2 = 2;\n"
                           "}\n"
                           "}\n";
  SourceFileManager SFM;
  SFM.createFile("test.cpp", Code);
  std::vector<std::string> Srcs = {SFM.getFilePath("test.cpp")};
  ASSERT_TRUE(init(SFM.getBaseDirPath(), Srcs));

  auto RNode = generateRDNode("test", 0, 3, -1);
  ASSERT_TRUE(RNode);

  std::vector<RDNode> RNodes = {*RNode};
  auto DG = getDefMapGenerator(RNodes);

  RNode = generateRDNode("test", 0, 5, -1);
  ASSERT_TRUE(RNode);
  auto ANode = generateASTIRNode(*RNode);
  ASSERT_TRUE(ANode);
  ASSERT_THROW(DG.getID(ANode->AST), std::invalid_argument);
}
