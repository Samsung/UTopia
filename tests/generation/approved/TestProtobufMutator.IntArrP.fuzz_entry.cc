#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <algorithm>
extern "C" {
int *autofuzz0;
unsigned autofuzz0size;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0[10 + 1] = {};
  autofuzz0size = 10 <= autofuzz_mutation.fuzzvar0().size()
                      ? 10
                      : autofuzz_mutation.fuzzvar0().size();
  std::copy(autofuzz_mutation.fuzzvar0(),
            autofuzz_mutation.fuzzvar0() + autofuzz0size, fuzzvar0);
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}