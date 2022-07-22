#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
int autofuzz0;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0;
  fuzzvar0 = autofuzz_mutation.fuzzvar0();
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}