#include "TestHelper.h"
#include "ftg/utanalysis/TargetAPIFinder.h"
#include <gtest/gtest.h>

using namespace llvm;
using namespace ftg;

#define LOAD(CODE, SourceType)                                                 \
  auto CH = TestHelperFactory().createCompileHelper(CODE, "test_api",          \
                                                    "-O0 -g", SourceType);     \
  ASSERT_TRUE(CH);                                                             \
  auto SC = CH->load();                                                        \
  ASSERT_TRUE(SC);                                                             \
  IRAccessHelper IRAccess(SC->getLLVMModule());

TEST(TargetAPIFinder, BasicP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "  API_2();\n"
                           "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API_1");
  auto *API2 = IRAccess.getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAccess.getFunction("test");
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
  auto *Caller = IRAccess.getInstruction("test", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAccess.getInstruction("test", 0, 1);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST(TargetAPIFinder, NullAPIN) {

  const std::string CODE = "void API_1();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *Test = IRAccess.getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {nullptr};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 0);
}

TEST(TargetAPIFinder, NullFuncN) {

  const std::string CODE = "void API_1();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API_1");
  ASSERT_TRUE(API1);

  std::set<llvm::Function *> APIFuncs = {API1};
  std::vector<llvm::Function *> Funcs = {nullptr};

  try {
    auto TargetAPIs = TargetAPIFinder(APIFuncs).findAPIs(Funcs);
    ASSERT_TRUE(false);
  } catch (std::invalid_argument &Exc) {
  }
}

TEST(TargetAPIFinder, NoDupP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test() {\n"
                           "  API_1();\n"
                           "  API_1();\n"
                           "  API_2();\n"
                           "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API_1");
  auto *API2 = IRAccess.getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAccess.getFunction("test");
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
  auto *Caller = IRAccess.getInstruction("test", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAccess.getInstruction("test", 0, 1);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAccess.getInstruction("test", 0, 2);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST(TargetAPIFinder, MultiFuncP) {

  const std::string CODE = "void API_1();\n"
                           "void API_2();\n"
                           "void API_3();\n"
                           "void test_1() {\n"
                           "  API_1();\n"
                           "}\n"
                           "void test_2() {\n"
                           "  API_2();\n"
                           "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API_1");
  auto *API2 = IRAccess.getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test1 = IRAccess.getFunction("test_1");
  auto *Test2 = IRAccess.getFunction("test_2");
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
  Instruction *Caller = IRAccess.getInstruction("test_1", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAccess.getInstruction("test_2", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST(TargetAPIFinder, IndirectCallN) {

  const char *CODE = "void API_1();\n"
                     "void API_2();\n"
                     "void test() {\n"
                     "  void (*FPtr)();\n"
                     "  FPtr = &API_1;\n"
                     "  FPtr();\n"
                     "  API_2();\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API_1");
  auto *API2 = IRAccess.getFunction("API_2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAccess.getFunction("test");
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
  auto *BB = IRAccess.getBasicBlock("test", 0);
  ASSERT_TRUE(BB);

  llvm::Instruction *I = nullptr;
  if (BB->size() == 7)
    I = IRAccess.getInstruction("test", 0, 5);
  else if (BB->size() == 8)
    I = IRAccess.getInstruction("test", 0, 6);
  ASSERT_TRUE(I);

  auto *CB = llvm::dyn_cast_or_null<CallBase>(I);
  ASSERT_TRUE(CB);
  ASSERT_NE(APICallers.find(CB), APICallers.end());
}

TEST(TargetAPIFinder, SubroutineP) {

  const char *CODE = "void API();\n"
                     "void sub() {\n"
                     "  API();\n"
                     "}\n"
                     "void test() {\n"
                     "  sub();\n"
                     "}\n";

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API = IRAccess.getFunction("API");
  ASSERT_TRUE(API);

  auto *Test = IRAccess.getFunction("test");
  ASSERT_TRUE(Test);

  std::set<llvm::Function *> APIFuncs = {API};
  std::vector<llvm::Function *> Funcs = {Test};
  auto Finder = TargetAPIFinder(APIFuncs);
  auto TargetAPIs = Finder.findAPIs(Funcs);
  ASSERT_EQ(TargetAPIs.size(), 1);
  ASSERT_NE(TargetAPIs.find(API), TargetAPIs.end());

  auto APICallers = Finder.findAPICallers(Funcs);
  ASSERT_EQ(APICallers.size(), 1);
  auto *Caller = IRAccess.getInstruction("sub", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}

TEST(TargetAPIFinder, RecursionP) {

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

  LOAD(CODE, CompileHelper::SourceType_C);

  auto *API1 = IRAccess.getFunction("API1");
  auto *API2 = IRAccess.getFunction("API2");
  ASSERT_TRUE(API1);
  ASSERT_TRUE(API2);

  auto *Test = IRAccess.getFunction("test");
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
  auto *Caller = IRAccess.getInstruction("sub1", 0, 2);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());

  Caller = IRAccess.getInstruction("sub2", 0, 0);
  ASSERT_TRUE(Caller);
  ASSERT_TRUE(isa<CallBase>(Caller));
  ASSERT_NE(APICallers.find(dyn_cast<CallBase>(Caller)), APICallers.end());
}
