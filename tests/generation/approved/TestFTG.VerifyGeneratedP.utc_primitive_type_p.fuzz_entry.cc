#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
extern "C" {
int autofuzz22;
unsigned int autofuzz23;
char autofuzz24;
float autofuzz25;
double autofuzz26;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar22;
  fuzzvar22 = autofuzz_mutation.fuzzvar22();
  autofuzz22 = fuzzvar22;
  unsigned int fuzzvar23;
  fuzzvar23 = autofuzz_mutation.fuzzvar23();
  autofuzz23 = fuzzvar23;
  char fuzzvar24;
  fuzzvar24 = autofuzz_mutation.fuzzvar24();
  autofuzz24 = fuzzvar24;
  float fuzzvar25;
  fuzzvar25 = autofuzz_mutation.fuzzvar25();
  autofuzz25 = fuzzvar25;
  double fuzzvar26;
  fuzzvar26 = autofuzz_mutation.fuzzvar26();
  autofuzz26 = fuzzvar26;
  enterAutofuzz();
}