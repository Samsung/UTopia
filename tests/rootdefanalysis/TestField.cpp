#include "TestHelper.h"
#include "ftg/rootdefanalysis/RDAnalyzer.h"
#include "ftg/rootdefanalysis/RDNode.h"
#include <gtest/gtest.h>

using namespace ftg;
using namespace llvm;

#define LOAD(CODE, SourceType)                                                 \
  auto CH = TestHelperFactory().createCompileHelper(CODE, "test_rd", "-O0 -g", \
                                                    SourceType);               \
  ASSERT_TRUE(CH);                                                             \
  auto SC = CH->load();                                                        \
  ASSERT_TRUE(SC);                                                             \
  IRAccessHelper IRAccess(SC->getLLVMModule());

#define INST(Inst, Func, BIdx, IIdx)                                           \
  do {                                                                         \
    Inst = IRAccess.getInstruction(Func, BIdx, IIdx);                          \
    ASSERT_TRUE(Inst);                                                         \
  } while (0)

#define SETUP(Analyzer, ...)                                                   \
  do {                                                                         \
    std::vector<llvm::Function *> Funcs;                                       \
    std::vector<std::string> Names = {__VA_ARGS__};                            \
    for (auto Name : Names) {                                                  \
      auto *F = IRAccess.getFunction(Name);                                    \
      ASSERT_TRUE(F);                                                          \
      Funcs.push_back(F);                                                      \
    }                                                                          \
    Analyzer.setSearchSpace(Funcs);                                            \
  } while (0)

#define FIND(Defs, Analyzer, Func, BIdx, IIdx, AIdx)                           \
  do {                                                                         \
    Instruction *I = nullptr;                                                  \
    INST(I, Func, BIdx, IIdx);                                                 \
    Defs = Analyzer.getRootDefinitions(I->getOperandUse(AIdx));                \
  } while (0)

#define CHECK_EXIST(RDefs, I, Idx)                                             \
  do {                                                                         \
    bool Found = false;                                                        \
    for (auto &RDef : RDefs) {                                                 \
      auto Def = RDef.getDefinition();                                         \
      if (Def.first == I && Def.second == Idx) {                               \
        Found = true;                                                          \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    ASSERT_TRUE(Found);                                                        \
  } while (0)

TEST(RDField, ArrayIndexN) {

  const char *CODE = "void test() {\n"
                     "  int Var1[10];\n"
                     "  Var1[3] = 20;\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  Instruction *I = nullptr, *Base = nullptr;
  INST(I, "test", 0, 3);
  INST(Base, "test", 0, 0);

  RDNode Node(1, *I);
  auto &Target = Node.getTarget();
  auto &TargetBase = Target.getIR();
  auto &MemoryIndices = Target.getMemoryIndices();

  ASSERT_EQ(&TargetBase, Base);
  ASSERT_EQ(MemoryIndices.size(), 1);
  ASSERT_EQ(MemoryIndices[0], 3);
}

TEST(RDField, MultiArrayIndexN) {

  const char *CODE = "void test() {\n"
                     "  int Var1[10][10];\n"
                     "  Var1[5][3] = 20;\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  Instruction *I = nullptr, *Base = nullptr;
  INST(I, "test", 0, 4);
  INST(Base, "test", 0, 0);

  RDNode Node(1, *I);
  auto &Target = Node.getTarget();
  auto &TargetBase = Target.getIR();
  auto &MemoryIndices = Target.getMemoryIndices();

  ASSERT_EQ(&TargetBase, Base);
  ASSERT_EQ(MemoryIndices.size(), 2);
  ASSERT_EQ(MemoryIndices[0], 5);
  ASSERT_EQ(MemoryIndices[1], 3);
}

TEST(RDField, StructIndexN) {

  const char *CODE = "struct ST { int F1; int F2; int F3; };\n"
                     "void test() {\n"
                     "  struct ST Var1;\n"
                     "  Var1.F2 = 20;\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  Instruction *I = nullptr, *Base = nullptr;
  INST(I, "test", 0, 3);
  INST(Base, "test", 0, 0);

  RDNode Node(1, *I);
  auto &Target = Node.getTarget();
  auto &TargetBase = Target.getIR();
  auto &MemoryIndices = Target.getMemoryIndices();

  ASSERT_EQ(&TargetBase, Base);
  ASSERT_EQ(MemoryIndices.size(), 1);
  ASSERT_EQ(MemoryIndices[0], 1);
}

TEST(RDField, NestedStructIndexN) {

  const char *CODE = "struct ST1 { int F1; int F2; int F3; };\n"
                     "struct ST2 { int F1; struct ST1 F2; };\n"
                     "void test() {\n"
                     "  struct ST2 Var1;\n"
                     "  Var1.F2.F2 = 20;\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  Instruction *I = nullptr, *Base = nullptr;
  INST(I, "test", 0, 4);
  INST(Base, "test", 0, 0);

  RDNode Node(1, *I);
  auto &Target = Node.getTarget();
  auto &TargetBase = Target.getIR();
  auto &MemoryIndices = Target.getMemoryIndices();

  ASSERT_EQ(&TargetBase, Base);
  ASSERT_EQ(MemoryIndices.size(), 2);
  ASSERT_EQ(MemoryIndices[0], 1);
  ASSERT_EQ(MemoryIndices[1], 1);
}

TEST(RDField, PointerIndexN) {

  const char *CODE = "void test() {\n"
                     "  int Var1[10];\n"
                     "  int *Var2 = Var1;\n"
                     "  Var2[7] = 20;\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  Instruction *I = nullptr, *Base = nullptr;
  INST(I, "test", 0, 8);
  INST(Base, "test", 0, 1);

  RDNode Node(1, *I);
  auto &Target = Node.getTarget();
  auto &TargetBase = Target.getIR();
  auto &MemoryIndices = Target.getMemoryIndices();

  ASSERT_EQ(&TargetBase, Base);
  ASSERT_EQ(MemoryIndices.size(), 1);
  ASSERT_EQ(MemoryIndices[0], 7);
}

TEST(RDField, ExactFieldN) {

  const char *CODE = "void API(int);\n"
                     "struct ST { int F1; int F2; int F3; };\n"
                     "void test() {\n"
                     "  struct ST Var1;\n"
                     "  Var1.F1 = 10;\n"
                     "  Var1.F2 = 20;\n"
                     "  Var1.F3 = 30;\n"
                     "  API(Var1.F1);\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  RDAnalyzer Analyzer;
  SETUP(Analyzer, "test");

  std::set<RDNode> RDefs;
  FIND(RDefs, Analyzer, "test", 0, 10, 0);

  ASSERT_EQ(RDefs.size(), 1);

  Instruction *I = nullptr;
  INST(I, "test", 0, 3);

  auto Def = RDefs.begin()->getDefinition();
  ASSERT_EQ(Def.first, I);
  ASSERT_EQ(Def.second, -1);
}

TEST(RDField, InclusiveFieldN) {

  const char *CODE = "void API(int);\n"
                     "struct ST1 { int F1; int F2; };\n"
                     "struct ST2 { struct ST1 F1; int F2; };\n"
                     "void test() {\n"
                     "  struct ST2 Var1;\n"
                     "  struct ST1 Var2 = { 0, 20 };\n"
                     "  Var1.F1 = Var2;\n"
                     "  API(Var1.F1.F2);\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  RDAnalyzer Analyzer;
  SETUP(Analyzer, "test");

  std::set<RDNode> RDefs;
  FIND(RDefs, Analyzer, "test", 0, 13, 0);

  ASSERT_EQ(RDefs.size(), 1);

  Instruction *I = nullptr;
  INST(I, "test", 0, 5);

  auto Def = RDefs.begin()->getDefinition();
  ASSERT_EQ(Def.first, I);
  ASSERT_EQ(Def.second, -1);
}

TEST(RDField, IncludedFieldN) {

  const char *CODE = "void API(struct ST*);\n"
                     "struct ST { int F1; int F2; };\n"
                     "void test() {\n"
                     "  struct ST Var1 = { 10, 20 };\n"
                     "  Var1.F1 = 30;\n"
                     "  API(&Var1);\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  RDAnalyzer Analyzer;
  SETUP(Analyzer, "test");

  std::set<RDNode> RDefs;
  FIND(RDefs, Analyzer, "test", 0, 7, 0);

  ASSERT_EQ(RDefs.size(), 2);

  Instruction *I = nullptr;
  INST(I, "test", 0, 3);
  CHECK_EXIST(RDefs, I, -1);

  INST(I, "test", 0, 5);
  CHECK_EXIST(RDefs, I, -1);
}
