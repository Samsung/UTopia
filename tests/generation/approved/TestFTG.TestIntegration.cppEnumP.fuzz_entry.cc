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
 *  .:: .:::      ...   .....::     .::.  Base UT: Test_Enum_Test
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
#include "autofuzz.h"
extern "C" {
E1 autofuzz0;
E2 autofuzz1;
E2 autofuzz2;
E2 autofuzz3;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  E1 fuzzvar0;
  fuzzvar0 = static_cast<E1>(autofuzz_mutation.fuzzvar0());
  autofuzz0 = fuzzvar0;
  E2 fuzzvar1;
  fuzzvar1 = static_cast<E2>(autofuzz_mutation.fuzzvar1());
  autofuzz1 = fuzzvar1;
  E2 fuzzvar2;
  fuzzvar2 = static_cast<E2>(autofuzz_mutation.fuzzvar2());
  autofuzz2 = fuzzvar2;
  E2 fuzzvar3;
  fuzzvar3 = static_cast<E2>(autofuzz_mutation.fuzzvar3());
  autofuzz3 = fuzzvar3;
  enterAutofuzz();
}