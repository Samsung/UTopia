#include "FuzzArgsProfile.pb.h"
#include "libprotobuf-mutator/src/libfuzzer/libfuzzer_macro.h"
#include <fcntl.h>
#include <unistd.h>
#include "autofuzz.h"
extern "C" {
char *autofuzz0;
}
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  char *fuzzvar0;
  std::string fuzzvar0_filepath(FUZZ_FILEPATH_PREFIX +
                                std::string("fuzzvar0_file"));
  int fuzzvar0_fd =
      open(fuzzvar0_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fuzzvar0_fd != -1) {
    write(fuzzvar0_fd, autofuzz_mutation.fuzzvar0().c_str(),
          autofuzz_mutation.fuzzvar0().size());
    close(fuzzvar0_fd);
  }
  fuzzvar0 = const_cast<char *>(fuzzvar0_filepath.c_str());
  autofuzz0 = fuzzvar0;
  enterAutofuzz();
}