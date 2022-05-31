#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
extern "C" {
int autofuzz0;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0;
  fuzzvar0 = autofuzz_mutation.fuzzvar0();
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}