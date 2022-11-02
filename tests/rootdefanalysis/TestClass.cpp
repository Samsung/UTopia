#include "TestHelper.h"
#include "ftg/astirmap/DebugInfoMap.h"
#include "ftg/rootdefanalysis/RDAnalyzer.h"
#include "ftg/utils/ASTUtil.h"
#include "ftg/utils/LLVMUtil.h"
#include "llvm/IR/Function.h"

using namespace ftg;

class TestClass : public TestBase {

protected:
  void SetUp() {
    const std::string CODE = "#include <vector>\n"
                             "#include <string>\n"
                             "class CLS1 {\n"
                             "public:\n"
                             "  CLS1();\n"
                             "  CLS1(int A);\n"
                             "  void set(int A);\n"
                             "private:\n"
                             "  int F1;\n"
                             "};\n"
                             "void API_1(std::vector<int> P1);\n"
                             "void API_2(CLS1 P1);\n"
                             "void API_3(std::string P1);\n"
                             "void test_vector() {\n"
                             "  std::vector<int> Var1;\n"
                             "  Var1.push_back(20);\n"
                             "  Var1.push_back(10);\n"
                             "  API_1(Var1);\n"
                             "}\n"
                             "void test_class() {\n"
                             "  CLS1 Var1;\n"
                             "  Var1.set(10);\n"
                             "  Var1.set(20);\n"
                             "  API_2(Var1);\n"
                             "}\n"
                             "void test_string() {\n"
                             "  API_3(\"Hello\");\n"
                             "  API_3(std::string(\"Hello\"));\n"
                             "}\n";

    ASSERT_TRUE(loadCPP(CODE));
  }

  std::set<RDNode> analyze(std::string FuncName, unsigned BIdx, unsigned IIdx,
                           unsigned OIdx) {
    auto *I = IRAH->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return {};

    return analyze(*I, OIdx);
  }

  std::set<RDNode> analyze(llvm::Instruction &I, unsigned OIdx) {
    RDExtension Extension;
    for (const auto *Method :
         util::collectNonStaticClassMethods(SC->getASTUnits())) {
      for (auto &MangledName : util::getMangledNames(*Method)) {
        Extension.addNonStaticClassMethod(MangledName);
      }
    }
    RDAnalyzer Analyzer(0, &Extension);

    auto *F = I.getFunction();
    if (!F)
      return {};

    std::vector<llvm::Function *> FuncNames = {F};
    Analyzer.setSearchSpace(FuncNames);

    return Analyzer.getRootDefinitions(I.getOperandUse(OIdx));
  }

  bool exist(const std::set<RDNode> &Nodes, std::string FuncName, unsigned BIdx,
             unsigned IIdx, int OIdx) const {

    auto *I = IRAH->getInstruction(FuncName, BIdx, IIdx);
    if (!I)
      return false;

    for (auto &Node : Nodes) {
      auto Def = Node.getDefinition();
      if (Def.first != I || Def.second != OIdx)
        continue;

      return true;
    }

    return false;
  }

  /*
   *  To get a callbase instruction by given a function name, a basic block
   * index, and called function name. Note that, this function will return an
   *  instruction that meets first even if there are more than one instruction
   *  that is satisfied with a given condition.
   */
  llvm::Instruction *getCallBaseByCalledFunctionName(std::string FIdx,
                                                     unsigned BIdx,
                                                     std::string FuncName) {

    auto *B = IRAH->getBasicBlock(FIdx, BIdx);
    if (!B)
      return nullptr;

    for (auto &I : *B) {
      auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(&I);
      if (!CB)
        continue;

      auto *CF = util::getCalledFunction(*CB);
      if (!CF || CF->getName() != FuncName)
        continue;

      return CB;
    }

    return nullptr;
  }
};

TEST_F(TestClass, TestVectorN) {

  auto RootDefs = analyze("_Z11test_vectorv", 3, 0, 0);

  //(1) Var1.push_back(20);
  //(2) Var1.push_back(10);
  ASSERT_TRUE(exist(RootDefs, "_Z11test_vectorv", 0, 8, -1));
  ASSERT_TRUE(exist(RootDefs, "_Z11test_vectorv", 1, 0, -1));
}

TEST_F(TestClass, TestClassExternMethodN) {

  // NOTE: clang 10.0.0 emits IR for API_2(Var1); like below:
  //         call void @_Z5API_24CLS1(%class.CLS1* byval(%class.CLS1)
  //       Otherwise, clang 10.0.1 emits like below:
  //           %5 = getelementptr inbounds %class.CLS1, %class.CLS1* %2, i32 0,
  //               i32 0
  //           %6 = load i32, i32* %5, align 4
  //           call void @_Z5API_24CLS1(i32 %6)
  //       This causes difference of the number of instructions in emitted IR.
  //       Thus below getInstruction function may give different result because
  //       a third argument of this call is related to this difference.
  // TODO: find more elegant approach to distinguish the difference of
  //       the emitted IR. So far, the number of instructions in related basic
  //       block is used to distinguish them explicitly as a heuristic approach.
  auto *I =
      getCallBaseByCalledFunctionName("_Z10test_classv", 0, "_Z5API_24CLS1");
  ASSERT_TRUE(I);

  auto RootDefs = analyze(*I, 0);
  ASSERT_TRUE(exist(RootDefs, "_Z10test_classv", 0, 4, 1));
  ASSERT_TRUE(exist(RootDefs, "_Z10test_classv", 0, 5, 1));
}

TEST_F(TestClass, TestStringN) {

  auto RootDefs = analyze("_Z11test_stringv", 1, 0, 0);
  ASSERT_TRUE(exist(RootDefs, "_Z11test_stringv", 0, 7, 1));

  RootDefs = analyze("_Z11test_stringv", 3, 0, 0);
  ASSERT_TRUE(exist(RootDefs, "_Z11test_stringv", 2, 3, 1));
}
