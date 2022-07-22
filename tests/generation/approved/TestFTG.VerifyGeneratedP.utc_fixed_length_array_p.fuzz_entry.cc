#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <algorithm>
#include "autofuzz.h"
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
char *autofuzz20;
unsigned autofuzz20size;
int autofuzz21;
int *autofuzz22;
unsigned autofuzz22size;
int *autofuzz23;
unsigned autofuzz23size;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  int fuzzvar0[20 + 1] = {};
  autofuzz0size = 20 <= autofuzz_mutation.fuzzvar0().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar0().size();
  std::copy(autofuzz_mutation.fuzzvar0().begin(),
            autofuzz_mutation.fuzzvar0().begin() + autofuzz0size, fuzzvar0);
  autofuzz0 = fuzzvar0;
  int fuzzvar1[20 + 1] = {};
  autofuzz1size = 20 <= autofuzz_mutation.fuzzvar1().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar1().size();
  std::copy(autofuzz_mutation.fuzzvar1().begin(),
            autofuzz_mutation.fuzzvar1().begin() + autofuzz1size, fuzzvar1);
  autofuzz1 = fuzzvar1;
  int fuzzvar2[20 + 1] = {};
  autofuzz2size = 20 <= autofuzz_mutation.fuzzvar2().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar2().size();
  std::copy(autofuzz_mutation.fuzzvar2().begin(),
            autofuzz_mutation.fuzzvar2().begin() + autofuzz2size, fuzzvar2);
  autofuzz2 = fuzzvar2;
  int fuzzvar3[20 + 1] = {};
  autofuzz3size = 20 <= autofuzz_mutation.fuzzvar3().size()
                      ? 20
                      : autofuzz_mutation.fuzzvar3().size();
  std::copy(autofuzz_mutation.fuzzvar3().begin(),
            autofuzz_mutation.fuzzvar3().begin() + autofuzz3size, fuzzvar3);
  autofuzz3 = fuzzvar3;
  int fuzzvar13[20 + 1] = {};
  autofuzz13size = 20 <= autofuzz_mutation.fuzzvar13().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar13().size();
  std::copy(autofuzz_mutation.fuzzvar13().begin(),
            autofuzz_mutation.fuzzvar13().begin() + autofuzz13size, fuzzvar13);
  autofuzz13 = fuzzvar13;
  int fuzzvar14[20 + 1] = {};
  autofuzz14size = 20 <= autofuzz_mutation.fuzzvar14().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar14().size();
  std::copy(autofuzz_mutation.fuzzvar14().begin(),
            autofuzz_mutation.fuzzvar14().begin() + autofuzz14size, fuzzvar14);
  autofuzz14 = fuzzvar14;
  int fuzzvar15[50 + 1] = {};
  autofuzz15size = 50 <= autofuzz_mutation.fuzzvar15().size()
                       ? 50
                       : autofuzz_mutation.fuzzvar15().size();
  std::copy(autofuzz_mutation.fuzzvar15().begin(),
            autofuzz_mutation.fuzzvar15().begin() + autofuzz15size, fuzzvar15);
  autofuzz15 = fuzzvar15;
  char fuzzvar16[50 + 1] = {};
  autofuzz16size = 50 <= autofuzz_mutation.fuzzvar16().size()
                       ? 50
                       : autofuzz_mutation.fuzzvar16().size();
  std::copy(autofuzz_mutation.fuzzvar16().begin(),
            autofuzz_mutation.fuzzvar16().begin() + autofuzz16size, fuzzvar16);
  autofuzz16 = fuzzvar16;
  char fuzzvar17[5 + 1] = {};
  autofuzz17size = 5 <= autofuzz_mutation.fuzzvar17().size()
                       ? 5
                       : autofuzz_mutation.fuzzvar17().size();
  std::copy(autofuzz_mutation.fuzzvar17().begin(),
            autofuzz_mutation.fuzzvar17().begin() + autofuzz17size, fuzzvar17);
  autofuzz17 = fuzzvar17;
  int fuzzvar18[20 + 1] = {};
  autofuzz18size = 20 <= autofuzz_mutation.fuzzvar18().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar18().size();
  std::copy(autofuzz_mutation.fuzzvar18().begin(),
            autofuzz_mutation.fuzzvar18().begin() + autofuzz18size, fuzzvar18);
  autofuzz18 = fuzzvar18;
  int fuzzvar19;
  fuzzvar19 = autofuzz_mutation.fuzzvar18().size() <= 20
                  ? autofuzz_mutation.fuzzvar18().size()
                  : 20;
  autofuzz19 = fuzzvar19;
  char fuzzvar20[20 + 1] = {};
  autofuzz20size = 20 <= autofuzz_mutation.fuzzvar20().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar20().size();
  std::copy(autofuzz_mutation.fuzzvar20().begin(),
            autofuzz_mutation.fuzzvar20().begin() + autofuzz20size, fuzzvar20);
  autofuzz20 = fuzzvar20;
  int fuzzvar21;
  fuzzvar21 = autofuzz_mutation.fuzzvar20().size() <= 20
                  ? autofuzz_mutation.fuzzvar20().size()
                  : 20;
  autofuzz21 = fuzzvar21;
  int fuzzvar22[20 + 1] = {};
  autofuzz22size = 20 <= autofuzz_mutation.fuzzvar22().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar22().size();
  std::copy(autofuzz_mutation.fuzzvar22().begin(),
            autofuzz_mutation.fuzzvar22().begin() + autofuzz22size, fuzzvar22);
  autofuzz22 = fuzzvar22;
  int fuzzvar23[20 + 1] = {};
  autofuzz23size = 20 <= autofuzz_mutation.fuzzvar23().size()
                       ? 20
                       : autofuzz_mutation.fuzzvar23().size();
  std::copy(autofuzz_mutation.fuzzvar23().begin(),
            autofuzz_mutation.fuzzvar23().begin() + autofuzz23size, fuzzvar23);
  autofuzz23 = fuzzvar23;
  enterAutofuzz();
}