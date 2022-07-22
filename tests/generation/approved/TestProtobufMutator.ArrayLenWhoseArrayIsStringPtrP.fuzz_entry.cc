#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
char *autofuzz0;
unsigned autofuzz1;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar0;
  fuzzvar0 = const_cast<char *>(autofuzz_mutation.fuzzvar0().c_str());
  autofuzz0 = fuzzvar0;
  unsigned fuzzvar1;
  fuzzvar1 = autofuzz_mutation.fuzzvar0().size();
  autofuzz1 = fuzzvar1;
  enterAutofuzz();
}