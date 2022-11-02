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
 *  .:: .:::      ...   .....::     .::.  Base UT: utc_primitive_type_p
 *  .:: .:::      .::.  ..::::.     .::.
 *  .:: .::: .:.  .:::  :::..       .::.  This file was generated automatically
 *  .::. ... .::: .:::  ....        .::.  by UTopia v[version]
 *   .::::..  .:: .:::  .:::    ..:::..
 *      ..:::...   :::  ::.. .::::..
 *          ..:::.. ..  ...:::..
 *             ..::::..::::.
 *                 ..::..
 *****************************************************************************/
#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
int autofuzz26;
unsigned int autofuzz27;
char autofuzz28;
bool autofuzz29;
float autofuzz30;
double autofuzz31;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar26;
  fuzzvar26 = autofuzz_mutation.fuzzvar26();
  autofuzz26 = fuzzvar26;
  unsigned int fuzzvar27;
  fuzzvar27 = autofuzz_mutation.fuzzvar27();
  autofuzz27 = fuzzvar27;
  char fuzzvar28;
  fuzzvar28 = autofuzz_mutation.fuzzvar28();
  autofuzz28 = fuzzvar28;
  bool fuzzvar29;
  fuzzvar29 = autofuzz_mutation.fuzzvar29();
  autofuzz29 = fuzzvar29;
  float fuzzvar30;
  fuzzvar30 = autofuzz_mutation.fuzzvar30();
  autofuzz30 = fuzzvar30;
  double fuzzvar31;
  fuzzvar31 = autofuzz_mutation.fuzzvar31();
  autofuzz31 = fuzzvar31;
  enterAutofuzz();
}