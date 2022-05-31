#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
extern "C" {}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  enterAutofuzz();
}