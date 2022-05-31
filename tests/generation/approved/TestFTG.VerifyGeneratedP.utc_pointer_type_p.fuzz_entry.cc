#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
extern "C" {
int autofuzz29;
e1 autofuzz31;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar29;
  fuzzvar29 = autofuzz_mutation.fuzzvar29();
  autofuzz29 = fuzzvar29;
  e1 fuzzvar31;
  fuzzvar31 = autofuzz_mutation.fuzzvar31();
  autofuzz31 = fuzzvar31;
  enterAutofuzz();
}