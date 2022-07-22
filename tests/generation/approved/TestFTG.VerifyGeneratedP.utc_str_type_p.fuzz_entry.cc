#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
char *autofuzz32;
char *autofuzz33;
char *autofuzz35;
int autofuzz36;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar32;
  fuzzvar32 = const_cast<char *>(autofuzz_mutation.fuzzvar32().c_str());
  autofuzz32 = fuzzvar32;
  char *fuzzvar33;
  fuzzvar33 = const_cast<char *>(autofuzz_mutation.fuzzvar33().c_str());
  autofuzz33 = fuzzvar33;
  char *fuzzvar35;
  fuzzvar35 = const_cast<char *>(autofuzz_mutation.fuzzvar35().c_str());
  autofuzz35 = fuzzvar35;
  int fuzzvar36;
  fuzzvar36 = autofuzz_mutation.fuzzvar35().size();
  autofuzz36 = fuzzvar36;
  enterAutofuzz();
}