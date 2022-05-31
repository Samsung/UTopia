#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
extern "C" {
char *autofuzz27;
char *autofuzz28;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar27;
  fuzzvar27 = const_cast<char *>(autofuzz_mutation.fuzzvar27().c_str());
  autofuzz27 = fuzzvar27;
  char *fuzzvar28;
  fuzzvar28 = const_cast<char *>(autofuzz_mutation.fuzzvar28().c_str());
  autofuzz28 = fuzzvar28;
  enterAutofuzz();
}