#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "autofuzz.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <fcntl.h>
#include <unistd.h>
extern "C" {
char *autofuzz32;
int autofuzz33;
}
DEFINE_PROTO_FUZZER(const FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar32;
  std::string fuzzvar32_filepath(FUZZ_FILEPATH_PREFIX + fuzzvar32_file);
  int fuzzvar32_fd =
      open(fuzzvar32_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fuzzvar32_fd != -1) {
    write(fuzzvar32_fd, autofuzz_mutation.fuzzvar32().c_str(),
          autofuzz_mutation.fuzzvar32().size());
    close(fuzzvar32_fd);
  }
  fuzzvar32 = const_cast<char *>(fuzzvar32_filepath.c_str());
  autofuzz32 = fuzzvar32;
  int fuzzvar33;
  if (autofuzz_mutation.fuzzvar33() < 0)
    return;
  fuzzvar33 = autofuzz_mutation.fuzzvar33() & 0x3fff;
  autofuzz33 = fuzzvar33;
  enterAutofuzz();
}