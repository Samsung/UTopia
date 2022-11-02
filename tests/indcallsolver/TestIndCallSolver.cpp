#include "TestHelper.h"
#include "ftg/indcallsolver/GlobalInitializerSolver.h"
#include "ftg/indcallsolver/IndCallSolverMgr.h"
#include "ftg/indcallsolver/LLVMWalker.h"
#include "ftg/indcallsolver/TBAASimpleSolver.h"
#include "ftg/indcallsolver/TBAAVirtSolver.h"
#include "ftg/indcallsolver/TestTypeVirtSolver.h"

using namespace ftg;
using namespace llvm;

namespace {
const std::string CompileOpt = "-O1 -Xclang -disable-llvm-passes -flto "
                               "-fwhole-program-vtables -fvisibility=hidden";
}

class TestIndCallSolverMgr : public TestBase {
protected:
  bool found(IndCallSolverMgr &Solver, std::string FuncName, unsigned BIdx,
             unsigned IIdx, std::set<std::string> Answers) {
    auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
        IRAH->getInstruction(FuncName, BIdx, IIdx));
    if (!CB)
      return false;

    auto Result = Solver.getCalledFunctions(*CB);
    if (Result.size() != Answers.size())
      return false;

    for (const auto *F : Result) {
      if (!F)
        return false;

      if (Answers.find(F->getName()) == Answers.end())
        return false;
    }

    return true;
  }

  bool notfound(IndCallSolverMgr &Solver, std::string FuncName, unsigned BIdx,
                unsigned IIdx) {
    auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
        IRAH->getInstruction(FuncName, BIdx, IIdx));
    if (!CB)
      return false;

    auto Result = Solver.getCalledFunctions(*CB);
    return Result.size() == 0;
  }
};

TEST_F(TestIndCallSolverMgr, getCalledFunctionP) {
  std::string Code = "extern \"C\" {\n"
                     "void func1(void);\n"
                     "void func2(void);\n"
                     "void func3(int);\n"
                     "char func4(void);\n"
                     "char (*GVar)(void) = func4;\n"
                     "void func5(char);\n"
                     "void func6(char);\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M() { func5('a'); }\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M() { func6('b'); }\n"
                     "};\n"
                     "void test_direct_call() {\n"
                     "  func1();\n"
                     "}"
                     "void test_callee_metadata() {\n"
                     "  void (*Var1)(void) = func2;\n"
                     "  Var1();\n"
                     "}\n"
                     "void test_inst(void (*P)(int)) {\n"
                     "  void (*Var)(int) = func3;\n"
                     "  P(10);\n"
                     "}\n"
                     "void test_global(char (*P)(void)) {\n"
                     "  P();\n"
                     "}\n"
                     "void test_virt() {\n"
                     "  CLS1 *Var = new CLS2();\n"
                     "  Var->M();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));

  IndCallSolverMgr Solver;
  Solver.solve(*const_cast<Module *>(&SC->getLLVMModule()));

  ASSERT_TRUE(found(Solver, "test_direct_call", 0, 0, {"func1"}));
  ASSERT_TRUE(found(Solver, "test_callee_metadata", 0, 5, {"func2"}));
  ASSERT_TRUE(found(Solver, "test_inst", 0, 7, {"func3"}));
  ASSERT_TRUE(found(Solver, "test_global", 0, 3, {"func4"}));
  ASSERT_TRUE(
      found(Solver, "test_virt", 0, 18, {"_ZN4CLS11MEv", "_ZN4CLS21MEv"}));
}

TEST_F(TestIndCallSolverMgr, getCalledFunctionN) {
  std::string Code = "extern \"C\" {\n"
                     "void func1(void);\n"
                     "void func2(int);\n"
                     "void func3(char);\n"
                     "void func4(char);\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M() { func3('a'); }\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M() { func4('b'); }\n"
                     "};\n"
                     "void test_callee_metadata() {\n"
                     "  void (*Var1)(void) = func1;\n"
                     "  Var1();\n"
                     "}\n"
                     "void test_inst(void (*P)(int)) {\n"
                     "  void (*Var)(int) = func2;\n"
                     "  P(10);\n"
                     "}\n"
                     "void test_virt() {\n"
                     "  CLS1 *Var = new CLS2();\n"
                     "  Var->M();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, "-O0"));

  IndCallSolverMgr Solver;
  Solver.solve(*const_cast<Module *>(&SC->getLLVMModule()));
  llvm::outs() << SC->getLLVMModule() << "n";

  ASSERT_TRUE(notfound(Solver, "test_callee_metadata", 0, 3));
  ASSERT_TRUE(notfound(Solver, "test_inst", 0, 5));
  ASSERT_TRUE(notfound(Solver, "test_virt", 0, 13));
}

TEST_F(TestIndCallSolverMgr, getCalledFunctionWithNotExistCodeBaseN) {
  std::string AnalysisTargetCode =
      "typedef void(Func)(char *,int);\n\n"
      "void target(char *buf, int len, Func func) { func(buf, len); }\n\n";
  ASSERT_TRUE(loadC(AnalysisTargetCode, "-O1"));

  IndCallSolverMgr Solver;
  Solver.solve(*const_cast<Module *>(&SC->getLLVMModule()));

  std::string NewCode =
      "typedef void(Func)(char *,int);\n\n"
      "void newtarget(char *buf, int len, Func func) { func(buf, len); }\n\n";
  ASSERT_TRUE(loadC(NewCode, "-O1"));

  ASSERT_TRUE(notfound(Solver, "newtarget", 0, 0));
}

class TestIndCallSolverBase : public TestBase {
protected:
  bool found(IndCallSolver &Solver, std::string FuncName, unsigned BIdx,
             unsigned IIdx, std::set<std::string> Answers) {
    const auto *CB = llvm::dyn_cast_or_null<CallBase>(
        IRAH->getInstruction(FuncName, BIdx, IIdx));
    if (!CB)
      return false;

    auto Result = Solver.solve(*CB);
    if (Result.size() != Answers.size())
      return false;

    for (const auto *F : Result) {
      if (!F)
        return false;

      if (Answers.find(F->getName()) == Answers.end())
        return false;
    }
    return true;
  }

  template <typename T> void prepare(T &Handler) {
    LLVMWalker Walker;
    Walker.addHandler(&Handler);
    Walker.walk(SC->getLLVMModule());
  }
};

class TestGlobalInitializerSolver : public TestIndCallSolverBase {};

TEST_F(TestGlobalInitializerSolver, solveP) {
  std::string Code = "extern \"C\" {\n"
                     "int func1(void);\n"
                     "char func2(void);\n"
                     "float func3(void);\n"
                     "void func4(void);\n"
                     "void func5(void);\n"
                     "struct ST {\n"
                     "  char (*F1)(void);\n"
                     "  float (*F2)(void);\n"
                     "};\n"
                     "int (*GVar1)(void) = func1;\n"
                     "ST GVar2 = { .F1 = func2, .F2 = func3 };\n"
                     "void (*GVar3)(void) = func4;\n"
                     "void (*GVar4)(void) = func5;\n"
                     "void test_global_init(int (*P)(void)) {\n"
                     "  P();\n"
                     "}\n"
                     "void test_global_struct_init(ST *P) {\n"
                     "  P->F1();\n"
                     "  P->F2();\n"
                     "}\n"
                     "void test_two_inits(void (*P)(void)) {\n"
                     "  P();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));
  GlobalInitializerSolverHandler Handler;
  prepare(Handler);
  GlobalInitializerSolver Solver(std::move(Handler));
  ASSERT_TRUE(found(Solver, "test_global_init", 0, 3, {"func1"}));
  ASSERT_TRUE(found(Solver, "test_global_struct_init", 0, 5, {"func2"}));
  ASSERT_TRUE(found(Solver, "test_global_struct_init", 0, 9, {"func3"}));
  ASSERT_TRUE(found(Solver, "test_two_inits", 0, 3, {"func4", "func5"}));
}

TEST_F(TestGlobalInitializerSolver, solveN) {
  std::string Code = "extern \"C\" {\n"
                     "void func(void);\n"
                     "void (*Gvar)(void) = func;\n"
                     "void test_global_noninit(int (*P1)(void)) {\n"
                     "  P1();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));
  GlobalInitializerSolverHandler Handler;
  prepare(Handler);
  GlobalInitializerSolver Solver(std::move(Handler));
  ASSERT_TRUE(found(Solver, "test_global_noninit", 0, 3, {}));
}

class TestTBAASimpleSolver : public TestIndCallSolverBase {};

TEST_F(TestTBAASimpleSolver, solveP) {
  std::string Code = "extern \"C\" {\n"
                     "void func(int);\n"
                     "void test_store_and_load(void (*P)(int)) {\n"
                     "  void (*Var)(int) = func;\n"
                     "  P(10);\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));
  TBAASimpleSolverHandler TSSHandler;
  prepare(TSSHandler);
  TBAASimpleSolver Solver(std::move(TSSHandler));
  ASSERT_TRUE(found(Solver, "test_store_and_load", 0, 7, {"func"}));
}

TEST_F(TestTBAASimpleSolver, solveN) {
  std::string Code = "extern \"C\" {\n"
                     "void func(int);\n"
                     "void test_no_store(void (*P)(int)) {\n"
                     "  P(10);\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));
  TBAASimpleSolverHandler TSSHandler;
  prepare(TSSHandler);
  TBAASimpleSolver Solver(std::move(TSSHandler));
  ASSERT_TRUE(found(Solver, "test_no_store", 0, 3, {}));
}

class TestTestTypeVirtSolver : public TestIndCallSolverBase {};

TEST_F(TestTestTypeVirtSolver, solveP) {
  std::string Code = "extern \"C\" {\n"
                     "void func1();\n"
                     "void func2();\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M() { func1(); }\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M() { func2(); }\n"
                     "};\n"
                     "void test_virt() {\n"
                     "  CLS1 *Var = new CLS2();\n"
                     "  Var->M();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(Code, CompileOpt));
  TestTypeVirtSolverHandler Handler;
  prepare(Handler);
  Handler.collect(SC->getLLVMModule());
  TestTypeVirtSolver Solver(std::move(Handler));
  ASSERT_TRUE(
      found(Solver, "test_virt", 0, 18, {"_ZN4CLS11MEv", "_ZN4CLS21MEv"}));
}

TEST_F(TestTestTypeVirtSolver, solveN) {
  std::string Code = "extern \"C\" {\n"
                     "void func1();\n"
                     "void func2();\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M() { func1(); }\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M() { func2(); }\n"
                     "};\n"
                     "void test_virt() {\n"
                     "  CLS1 *Var = new CLS2();\n"
                     "  Var->M();\n"
                     "}\n"
                     "}";
  ASSERT_TRUE(loadCPP(
      Code, "-O1 -Xclang -disable-llvm-passes -flto -fwhole-program-vtables"));
  TestTypeVirtSolverHandler Handler;
  prepare(Handler);
  Handler.collect(SC->getLLVMModule());
  TestTypeVirtSolver Solver(std::move(Handler));
  ASSERT_TRUE(found(Solver, "test_virt", 0, 15, {}));
}

class TestTBAAVirtSolver : public TestIndCallSolverBase {};

TEST_F(TestTBAAVirtSolver, solveP) {
  std::string Code = "extern \"C\" {\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M();\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M();\n"
                     "};\n"
                     "class CLS3 {\n"
                     "public:\n"
                     "  void M1();\n"
                     "  virtual void M2(int P);\n"
                     "};\n"
                     "class CLS4 : public CLS3 {\n"
                     "public:\n"
                     "  void M2(int P);\n"
                     "};\n"
                     "void test() {\n"
                     "  CLS1 V1;\n"
                     "  CLS2 V2;\n"
                     "  CLS3 V3;\n"
                     "  CLS4 V4;\n"
                     "}\n"
                     "void test_two_virt(CLS1 *P1, CLS3 *P2) {\n"
                     "  P1->M();\n"
                     "  P2->M2(10);\n"
                     "}\n"
                     "}\n";
  ASSERT_TRUE(loadCPP(Code, "-O1 -Xclang -disable-llvm-passes"));
  TBAAVirtSolverHandler Handler;
  prepare(Handler);
  TBAAVirtSolver Solver(std::move(Handler));
  ASSERT_TRUE(
      found(Solver, "test_two_virt", 0, 9, {"_ZN4CLS11MEv", "_ZN4CLS21MEv"}));
  ASSERT_TRUE(found(Solver, "test_two_virt", 0, 15,
                    {"_ZN4CLS32M2Ei", "_ZN4CLS42M2Ei"}));
}

TEST_F(TestTBAAVirtSolver, solveN) {
  std::string Code = "extern \"C\" {\n"
                     "class CLS1 {\n"
                     "public:\n"
                     "  virtual void M();\n"
                     "};\n"
                     "class CLS2 : public CLS1 {\n"
                     "public:\n"
                     "  void M();\n"
                     "};\n"
                     "void test() {\n"
                     "  CLS1 V1;\n"
                     "  CLS2 V2;\n"
                     "}\n"
                     "void test_two_virt(CLS1 *P1) {\n"
                     "  P1->M();\n"
                     "}\n"
                     "}\n";
  ASSERT_TRUE(loadCPP(Code, "-O0"));
  TBAAVirtSolverHandler Handler;
  prepare(Handler);
  TBAAVirtSolver Solver(std::move(Handler));
  ASSERT_TRUE(found(Solver, "test_two_virt", 0, 7, {}));
}
