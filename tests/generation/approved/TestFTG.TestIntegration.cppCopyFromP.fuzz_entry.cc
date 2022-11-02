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
#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <algorithm>
#include "autofuzz.h"
extern "C" {
int *autofuzz0;
unsigned autofuzz0size;
int autofuzz1;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0[10 + 1] = {};
  autofuzz0size = 10 <= autofuzz_mutation.fuzzvar0().size()
                      ? 10
                      : autofuzz_mutation.fuzzvar0().size();
  std::copy(autofuzz_mutation.fuzzvar0().begin(),
            autofuzz_mutation.fuzzvar0().begin() + autofuzz0size, fuzzvar0);
  autofuzz0 = fuzzvar0;
  int fuzzvar1;
  fuzzvar1 = autofuzz_mutation.fuzzvar0().size() <= 10
                 ? autofuzz_mutation.fuzzvar0().size()
                 : 10;
  autofuzz1 = fuzzvar1;
  enterAutofuzz();
}