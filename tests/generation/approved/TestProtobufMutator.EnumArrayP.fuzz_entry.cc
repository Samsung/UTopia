#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
EnumT *autofuzz0;
unsigned autofuzz0size;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  fuzzvar0[10 + 1] = {};
  autofuzz0size = 10 <= autofuzz_mutation.fuzzvar0().size()
                      ? 10
                      : autofuzz_mutation.fuzzvar0().size();
  for (int i = 0; i < autofuzz0size; ++i) {
    fuzzvar0[i] = static_cast<EnumT>(autofuzz_mutation.fuzzvar0()[i]);
  }
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}