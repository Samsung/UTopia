#include "TestHelper.h"
#include "ftg/indcallsolver/IndCallSolver.h"
#include "ftg/indcallsolver/IndCallSolverImpl.h"

namespace ftg {
class TestIndCallSolver : public TestBase {
protected:
  bool found(IndCallSolver &Solver, std::string FuncName, unsigned BIdx,
             unsigned IIdx, std::string Answer = "") {
    auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
        IRAH->getInstruction(FuncName, BIdx, IIdx));
    if (!CB)
      return false;

    auto Result = Solver.getCalledFunction(*CB);
    if (!Result)
      return false;

    if (Answer.empty())
      return true;

    return Result->getName() == Answer;
  }

  bool notfound(IndCallSolver &Solver, std::string FuncName, unsigned BIdx,
                unsigned IIdx, std::string Answer = "") {
    auto *CB = llvm::dyn_cast_or_null<llvm::CallBase>(
        IRAH->getInstruction(FuncName, BIdx, IIdx));
    if (!CB)
      return false;

    auto Result = Solver.getCalledFunction(*CB);
    if (Answer.empty() && !Result)
      return true;

    return Result->getName() != Answer;
  }
};

TEST_F(TestIndCallSolver, getCalledFunctionP) {
  std::string Code = "extern \"C\" {\n"
                     "void func1(void);\n"
                     "void func2(void);\n"
                     "int func3(void);\n"
                     "char func4(void);\n"
                     "void func5(int);\n"
                     "int (*GVar1)(void) = func3;\n"
                     "struct ST1 {\n"
                     "  char (*F1)(void);\n"
                     "};\n"
                     "struct ST1 GVar2 = { .F1 = func4 };\n"
                     "void test_direct_call() {\n"
                     "  func1();\n"
                     "}"
                     "void test_callee_metadata() {\n"
                     "  void (*Var1)(void) = func2;\n"
                     "  Var1();\n"
                     "}\n"
                     "void test_global_init(int (*P1)(void)) {\n"
                     "  P1();\n"
                     "}\n"
                     "void test_field(struct ST1 *P1) {\n"
                     "  P1->F1();\n"
                     "}\n"
                     "void test_inst(void (*P1)(int)) {\n"
                     "  void (*Var)(int) = func5;\n"
                     "  P1(10);\n"
                     "}\n"
                     "}";
  // Two flags are needed for virtual class. -flto -fwhole-program-vtables
  ASSERT_TRUE(loadCPP(Code, "indcalltest", "-O1 -Xclang -disable-llvm-passes"));
  ASSERT_TRUE(CH);

  std::unique_ptr<IndCallSolver> Solver = std::make_unique<IndCallSolverImpl>();
  auto *M = CH->getLLVMModule();
  Solver->solve(*M);

  ASSERT_TRUE(found(*Solver, "test_direct_call", 0, 0, "func1"));
  ASSERT_TRUE(found(*Solver, "test_callee_metadata", 0, 5, "func2"));
  ASSERT_TRUE(found(*Solver, "test_global_init", 0, 3, "func3"));
  ASSERT_TRUE(found(*Solver, "test_field", 0, 5, "func4"));
  ASSERT_TRUE(found(*Solver, "test_inst", 0, 7, "func5"));
}

TEST_F(TestIndCallSolver, getCalledFunctionN) {
  std::string Code =
      "#include <memory>\n"
      "extern \"C\" {\n"
      "void func1(void);\n"
      "void func2(void);\n"
      "void func3(void);\n"
      "void (*Gvar1)(void) = func1;\n"
      "void (*Gvar2)(void) = func2;\n"
      "void (*Gvar3)(void) = func3;\n"
      "class CLS1 {\n"
      "public:\n"
      "  virtual ~CLS1() = default;\n"
      "  virtual void M() = 0;\n"
      "};\n"
      "class CLS2 : public CLS1 {\n"
      "public:\n"
      "  void M() override { F1 = 10; }\n"
      "private:\n"
      "  int F1;\n"
      "};\n"
      "void test_todo_twoinits(void (*P1)(void)) {\n"
      "  P1();\n"
      "}\n"
      "void test_virtual() {\n"
      "  std::shared_ptr<CLS1> Var1 = std::make_shared<CLS2>();\n"
      "  Var1->M();\n"
      "}\n"
      "}";
  ASSERT_TRUE(loadCPP(Code, "indcalltest", "-O1 -Xclang -disable-llvm-passes"));
  ASSERT_TRUE(CH);

  auto *M = CH->getLLVMModule();
  ASSERT_TRUE(M);

  auto Solver = std::make_unique<IndCallSolverImpl>();
  ASSERT_TRUE(Solver);
  Solver->solve(*M);

  ASSERT_TRUE(found(*Solver, "test_todo_twoinits", 0, 3));
  ASSERT_TRUE(notfound(*Solver, "test_virtual", 0, 19));
}

TEST_F(TestIndCallSolver, getCalledFunctionWithNotExistCodeBaseN) {
  std::string AnalysisTargetCode =
      "typedef void(Func)(char *,int);\n\n"
      "void target(char *buf, int len, Func func) { func(buf, len); }\n\n";
  ASSERT_TRUE(loadC(AnalysisTargetCode, "indcalltest", "-O1"));
  ASSERT_TRUE(CH);

  std::unique_ptr<IndCallSolver> Solver = std::make_unique<IndCallSolverImpl>();
  auto *M = CH->getLLVMModule();
  Solver->solve(*M);

  std::string NewCode =
      "typedef void(Func)(char *,int);\n\n"
      "void newtarget(char *buf, int len, Func func) { func(buf, len); }\n\n";
  ASSERT_TRUE(loadC(NewCode, "indcalltest", "-O1"));
  ASSERT_TRUE(CH);

  ASSERT_TRUE(notfound(*Solver, "newtarget", 0, 0));
}

} // namespace ftg
