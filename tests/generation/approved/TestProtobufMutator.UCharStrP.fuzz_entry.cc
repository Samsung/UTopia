#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include "autofuzz.h"
extern "C" {
unsigned char *autofuzz0;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  unsigned char *fuzzvar0;
  fuzzvar0 = reinterpret_cast<unsigned char *>(
      const_cast<char *>(autofuzz_mutation.fuzzvar0().c_str()));
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}