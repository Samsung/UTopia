#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <algorithm>
#include "autofuzz.h"
extern "C" {
int *autofuzz0;
unsigned autofuzz0size;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0[10 + 1] = {};
  autofuzz0size = 10 <= autofuzz_mutation.fuzzvar0().size()
                      ? 10
                      : autofuzz_mutation.fuzzvar0().size();
  std::copy(autofuzz_mutation.fuzzvar0().begin(),
            autofuzz_mutation.fuzzvar0().begin() + autofuzz0size, fuzzvar0);
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}