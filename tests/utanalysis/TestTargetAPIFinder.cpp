#include "TestHelper.h"
#include "ftg/utanalysis/TargetAPIFinder.h"
#include <gtest/gtest.h>

using namespace llvm;
using namespace ftg;

class TestTargetAPIFinder : public TestBase {};

TEST_F(TestTargetAPIFinder, BasicP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "  API_2();\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API_1");
  auto *API2 = IRAH->getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API1, API2};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 2);
  ASSERT_NE(TargetAPIs.find(API1), TargetAPIs.end());
  ASSERT_NE(TargetAPIs.find(API2), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 2);
  auto *Caller = IRAH->getInstruction("test", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAH->getInstruction("test", 0, 1);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST_F(TestTargetAPIFinder, NullAPIN) {

  const std::string CODE = "void API_1();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {nullptr};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 0);
}

TEST_F(TestTargetAPIFinder, NullFuncN) {

  const std::string CODE = "void API_1();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API_1");
  ASSERT_TRUE(API1);

  std::set<llvm::Function *> APIFuncs = {API1};
  std::vector<llvm::Function *> Funcs = {nullptr};

  try {
    auto TargetAPIs = TargetAPIFinder(APIFuncs).findAPIs(Funcs);
    ASSERT_TRUE(false);
  } catch (std::invalid_argument &Exc) {
  }
}

TEST_F(TestTargetAPIFinder, NoDupP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "  API_1();\n"
                           "  API_2();\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API_1");
  auto *API2 = IRAH->getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API1, API2};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 2);
  ASSERT_NE(TargetAPIs.find(API1), TargetAPIs.end());
  ASSERT_NE(TargetAPIs.find(API2), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 3);
  auto *Caller = IRAH->getInstruction("test", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAH->getInstruction("test", 0, 1);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAH->getInstruction("test", 0, 2);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST_F(TestTargetAPIFinder, MultiFuncP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test_1() {\n"
                           "  API_1();\n"
                           "}\n"
                           "void test_2() {\n"
                           "  API_2();\n"
                           "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API_1");
  auto *API2 = IRAH->getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test1 = IRAH->getFunction("test_1");
  auto *Test2 = IRAH->getFunction("test_2");
  ASSERT_TRUE(Test1);
  ASSERT_TRUE(Test2);

  std::set<llvm::Function *> APIFuncs = {API1, API2};
  std::vector<llvm::Function *> Funcs = {Test1, Test2};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 2);
  ASSERT_NE(TargetAPIs.find(API1), TargetAPIs.end());
  ASSERT_NE(TargetAPIs.find(API2), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 2);
  Instruction *Caller = IRAH->getInstruction("test_1", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAH->getInstruction("test_2", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST_F(TestTargetAPIFinder, IndirectCallN) {

  const char *CODE = "void API_1();\n"
                     "void API_2();\n"
                     "void test() {\n"
                     "  void (*FPtr)();\n"
                     "  FPtr = &API_1;\n"
                     "  FPtr();\n"
                     "  API_2();\n"
                     "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API_1");
  auto *API2 = IRAH->getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API1, API2};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);

  ASSERT_EQ(TargetAPIs.size(), 1);
  ASSERT_NE(TargetAPIs.find(API2), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 1);

  // NOTE: clang 10.0.0 emits IR for FPtr(); like below:
  //         %2 = load void (...)*, void (...)** %1, align 8, !dbg !17
  //         %3 = bitcast void (...)* %2 to void ()*, !dbg !20
  //         call void %3(), !dbg !20
  //       Otherwise, clang 10.0.1 emits like below:
  //         %2 = load void (...)*, void (...)** %1, align 8, !dbg !17
  //         call void (...) %2(), !dbg !17
  //       This causes difference of the number of instructions in emitted IR.
  //       Thus below getInstruction function may give different result because
  //       a third argument of this call is related to this difference.
  // TODO: find more elegant approach to distinguish the difference of
  //       the emitted IR. So far, the number of instructions in related basic
  //       block is used to distinguish them explicitly as a heuristic approach.
  auto *BB = IRAH->getBasicBlock("test", 0);
  ASSERT_TRUE(BB);

  llvm::Instruction *I = nullptr;
  if (BB->size() == 7)
    I = IRAH->getInstruction("test", 0, 5);
  else if (BB->size() == 8)
    I = IRAH->getInstruction("test", 0, 6);
  ASSERT_TRUE(I);

  auto *CB = llvm::dyn_cast_or_null<CallBase>(I);
  ASSERT_TRUE(CB);
  ASSERT_NE(APICallers.find(CB), APICallers.end());
}

TEST_F(TestTargetAPIFinder, SubroutineP) {

  const char *CODE = "void API();\n"
                     "void sub() {\n"
                     "  API();\n"
                     "}\n"
                     "void test() {\n"
                     "  sub();\n"
                     "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API = IRAH->getFunction("API");
  ASSERT_TRUE(API);

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 1);
  ASSERT_NE(TargetAPIs.find(API), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 1);
  auto *Caller = IRAH->getInstruction("sub", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST_F(TestTargetAPIFinder, RecursionP) {

  const char *CODE = "void API1();\n"
                     "void API2();\n"
                     "void sub2() {\n"
                     "  API2();\n"
                     "}\n"
                     "void sub1() {\n"
                     "  sub1();\n"
                     "  sub2();\n"
                     "  API1();\n"
                     "}\n"
                     "void test() {\n"
                     "  sub1();\n"
                     "}\n";

  ASSERT_TRUE(loadC(CODE));

  auto *API1 = IRAH->getFunction("API1");
  auto *API2 = IRAH->getFunction("API2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAH->getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API1, API2};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 2);
  ASSERT_NE(TargetAPIs.find(API1), TargetAPIs.end());
  ASSERT_NE(TargetAPIs.find(API2), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 2);
  auto *Caller = IRAH->getInstruction("sub1", 0, 2);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAH->getInstruction("sub2", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}
