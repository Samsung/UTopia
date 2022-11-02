/*****************************************************************************
*                 .::::.
*             ..:::...::::..
*         ..::::..      ..::::.
*      ..:::..              ..:::..
*   .::::.                      .::::.
*  .::.                            .::.
*  .::                         ..:. ::.  UTopia
*  .:: .::.                ..::::: .::.  Unit Tests to Fuzzing
*  .:: .:::             .::::::..  .::.  https://github.com/Samsung/UTopia
*  .:: .:::            ::::...     .::.
*  .:: .:::      ...   .....::     .::.  Base UT: Test_Group_Test
*  .:: .:::      .::.  ..::::.     .::.
*  .:: .::: .:.  .:::  :::..       .::.  This file was generated automatically
*  .::. ... .::: .:::  ....        .::.  by UTopia v[version]
*   .::::..  .:: .:::  .:::    ..:::..
*      ..:::...   :::  ::.. .::::..
*          ..:::.. ..  ...:::..
*             ..::::..::::.
*                 ..::..
*****************************************************************************/
#include "lib.h"
#include <gtest/gtest.h>
#include "autofuzz.h"
#ifdef __cplusplus
extern "C" {
#endif
extern int * autofuzz0;
extern unsigned autofuzz0size;
extern int autofuzz1;
#ifdef __cplusplus
}
#endif
#include "lib.h"
#include <gtest/gtest.h>
TEST(Test, Group) {
  int Var[10] = {0,}; { for (unsigned i=0; i<autofuzz0size; ++i) { Var[i] = autofuzz0[i]; } }
  if (Var[0] == 0) API(Var, autofuzz1);
  else API(Var, autofuzz1);
}

#ifdef __cplusplus
extern "C" {
#endif
void enterAutofuzz() {
  class AutofuzzTest : public Test_Group_Test {
  public:
    void runTest() {
      try {
        SetUpTestCase();
      } catch (std::exception &E) {}
      try {
        SetUp();
      } catch (std::exception &E) {}
      try {
        TestBody();
      } catch (std::exception &E) {}
      try {
        TearDown();
      } catch (std::exception &E) {}
      try {
        TearDownTestCase();
      } catch (std::exception &E) {}
    }
  };
  AutofuzzTest Fuzzer;
  Fuzzer.runTest();
}
#ifdef __cplusplus
}
#endif
