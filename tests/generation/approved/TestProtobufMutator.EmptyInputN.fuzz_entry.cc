#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  enterAutofuzz();
}