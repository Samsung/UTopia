#include "../lib/lib.h"
#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <fcntl.h>
#include <unistd.h>
#include "autofuzz.h"
extern "C" {
char *autofuzz40;
int autofuzz41;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar40;
  std::string fuzzvar40_filepath(FUZZ_FILEPATH_PREFIX + fuzzvar40_file);
  int fuzzvar40_fd =
      open(fuzzvar40_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fuzzvar40_fd != -1) {
    write(fuzzvar40_fd, autofuzz_mutation.fuzzvar40().c_str(),
          autofuzz_mutation.fuzzvar40().size());
    close(fuzzvar40_fd);
  }
  fuzzvar40 = const_cast<char *>(fuzzvar40_filepath.c_str());
  autofuzz40 = fuzzvar40;
  int fuzzvar41;
  if (autofuzz_mutation.fuzzvar41() < 0)
    return;
  fuzzvar41 = autofuzz_mutation.fuzzvar41() & 0x3fff;
  autofuzz41 = fuzzvar41;
  enterAutofuzz();
}