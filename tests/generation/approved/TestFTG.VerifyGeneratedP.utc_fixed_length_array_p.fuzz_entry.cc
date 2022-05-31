#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <algorithm>
extern "C" {
int *autofuzz0;
unsigned autofuzz0size;
int *autofuzz1;
unsigned autofuzz1size;
int *autofuzz2;
unsigned autofuzz2size;
int *autofuzz3;
unsigned autofuzz3size;
int *autofuzz13;
unsigned autofuzz13size;
int *autofuzz14;
unsigned autofuzz14size;
int *autofuzz15;
unsigned autofuzz15size;
char *autofuzz16;
unsigned autofuzz16size;
char *autofuzz17;
unsigned autofuzz17size;
int *autofuzz18;
unsigned autofuzz18size;
int autofuzz19;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0[20 + 1] = {};
  autofuzz0size = 20 <= autofuzz_mutation.fuzzvar0().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar0().size();
  std::copy(autofuzz_mutation.fuzzvar0(),
            autofuzz_mutation.fuzzvar0() + autofuzz0size, fuzzvar0);
  autofuzz0 = fuzzvar0;
  int fuzzvar1[20 + 1] = {};
  autofuzz1size = 20 <= autofuzz_mutation.fuzzvar1().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar1().size();
  std::copy(autofuzz_mutation.fuzzvar1(),
            autofuzz_mutation.fuzzvar1() + autofuzz1size, fuzzvar1);
  autofuzz1 = fuzzvar1;
  int fuzzvar2[20 + 1] = {};
  autofuzz2size = 20 <= autofuzz_mutation.fuzzvar2().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar2().size();
  std::copy(autofuzz_mutation.fuzzvar2(),
            autofuzz_mutation.fuzzvar2() + autofuzz2size, fuzzvar2);
  autofuzz2 = fuzzvar2;
  int fuzzvar3[20 + 1] = {};
  autofuzz3size = 20 <= autofuzz_mutation.fuzzvar3().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar3().size();
  std::copy(autofuzz_mutation.fuzzvar3(),
            autofuzz_mutation.fuzzvar3() + autofuzz3size, fuzzvar3);
  autofuzz3 = fuzzvar3;
  int fuzzvar13[20 + 1] = {};
  autofuzz13size = 20 <= autofuzz_mutation.fuzzvar13().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar13().size();
  std::copy(autofuzz_mutation.fuzzvar13(),
            autofuzz_mutation.fuzzvar13() + autofuzz13size, fuzzvar13);
  autofuzz13 = fuzzvar13;
  int fuzzvar14[20 + 1] = {};
  autofuzz14size = 20 <= autofuzz_mutation.fuzzvar14().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar14().size();
  std::copy(autofuzz_mutation.fuzzvar14(),
            autofuzz_mutation.fuzzvar14() + autofuzz14size, fuzzvar14);
  autofuzz14 = fuzzvar14;
  int fuzzvar15[50 + 1] = {};
  autofuzz15size = 50 <= autofuzz_mutation.fuzzvar15().size()
                       ? 50
                       : autofuzz_mutation.fuzzvar15().size();
  std::copy(autofuzz_mutation.fuzzvar15(),
            autofuzz_mutation.fuzzvar15() + autofuzz15size, fuzzvar15);
  autofuzz15 = fuzzvar15;
  char fuzzvar16[50 + 1] = {};
  autofuzz16size = 50 <= autofuzz_mutation.fuzzvar16().size()
                       ? 50
                       : autofuzz_mutation.fuzzvar16().size();
  std::copy(autofuzz_mutation.fuzzvar16(),
            autofuzz_mutation.fuzzvar16() + autofuzz16size, fuzzvar16);
  autofuzz16 = fuzzvar16;
  char fuzzvar17[5 + 1] = {};
  autofuzz17size = 5 <= autofuzz_mutation.fuzzvar17().size()
                       ? 5
                       : autofuzz_mutation.fuzzvar17().size();
  std::copy(autofuzz_mutation.fuzzvar17(),
            autofuzz_mutation.fuzzvar17() + autofuzz17size, fuzzvar17);
  autofuzz17 = fuzzvar17;
  int fuzzvar18[20 + 1] = {};
  autofuzz18size = 20 <= autofuzz_mutation.fuzzvar18().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar18().size();
  std::copy(autofuzz_mutation.fuzzvar18(),
            autofuzz_mutation.fuzzvar18() + autofuzz18size, fuzzvar18);
  autofuzz18 = fuzzvar18;
  int fuzzvar19;
  fuzzvar19 = autofuzz_mutation.fuzzvar18().size() <= 20
                  ? autofuzz_mutation.fuzzvar18().size()
                  : 20;
  autofuzz19 = fuzzvar19;
  enterAutofuzz();
}