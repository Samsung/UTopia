<p align="center">
  <img src="res/UTopia.png">
</p>

# Introduction

*UTopia* is a tool for automatically generating fuzz drivers from unit tests.

*UTopia* will let developers perform fuzz testing without special knowledge about writing fuzzers. Even developers
familiar with fuzzing can save significant time by generating fuzz drivers automatically.

*UTopia* supports C/C++ libraries which have unit tests with GoogleTest, Boost.Test or Tizen TCT.

# Trophy

To see bugs found by *UTopia*, visit [Trophy](Trophy.md) page. You can also see some *UTopia*-based fuzzers there.

# Docker

For easy set up, we provide docker image for running *UTopia*.
You can build *UTopia* and generate/run fuzzers inside docker container.

Build docker image with command below.

```shell
docker buildx build -f docker/Dockerfile -t utopia . #llvm-10
docker buildx build --build-arg LLVM_VERSION=12 -f docker/Dockerfile -t utopia . #llvm-12
```

*UTopia* is designed to work best with LLVM version 10. And, it has been confirmed to pass unit tests on LLVM version 12. So that we recommened you to use LLVM 10, but if you want you can try LLVM version 12.

# Build

*UTopia* depends on LLVM, Protobuf and GoogleTest. You can install dependencies manually, but we recommend to use given
docker image.

To build *UTopia*, follow cmake process below.

```shell
cd $UTOPIA_HOME_DIR
cmake -B build -S .
cmake --build build -j$(nproc)
```

# Run
For some selected projects, you can use helper script to run our tool without further effort. Please
refer [helper/README.md](helper/README.md).
For other projects, please check following manual.

## target_analyzer

Target Analyzer analyzes target library code and generates a result as json file.
Mandatory command line options are below.

```shell
target_analyzer --db ${builddb_path} --extern ${extern_path} --public ${api_json_path} --out ${output_path}
```

### builddb_path

Build db is a json file that contains paths of AST and IR files of target library code.
Its format looks like below.

```json
{
  "bc": "/root/fuzz-test-generation/exp/sample/output/bc/libcommon.a.bc",
  "ast": [
    "/root/fuzz-test-generation/exp/sample/libcommon.a_ast/codec/common/src/ast1.o.ast",
    "/root/fuzz-test-generation/exp/sample/libcommon.a_ast/codec/common/src/ast2.o.ast"
  ],
  "project_dir": "/root/fuzz-test-generation/exp/sample"
}
```

LLVM bitcode file path of a specific library should be specified using **"bc"** keyword.
We only accept one bitcode file so far, thus you may use `llvm-link` to link several bit code files
to one bit code file.

AST file paths of a specific library should be specified using **"ast"** keyword.
Note that a specified bitcode file and ast files are generated from same source codes for a specific library.

### extern_path

Target Analyzer accepts target analyzer report of other libraries for an accurate result.
This path should be a directory path where other reports stored, which means more than one library reports are allowed.

### api_json_path

API function names to be analyzed. It should be json file formatted as below.

```json
{
  "libcommon.a": [
    "API1",
    "API2",
    "API3"
  ]
}
```

You can get API list from a specific library using command below.

```shell
nm --no-demangle --defined-only -g ${librarypath} | awk '$2=="T" {k=""; for(i=3;i<=NF;i++) k=k $i""; print k}'
```

### Report

#### Direction

The Direction property is an essential parameter that the target analyzer utilizes. It signifies whether a parameter is employed for reading (**Dir_In**), writing (**Dir_Out**), or both reading and writing (**Dir_In** | **Dir_Out**) within a function. The target analyzer delineates this property through an enumeration type comprising the following elements:

```c++
enum Dir {
  Dir_NoOp = 0x000,        // No operation
  Dir_In = 0x100,          // Input direction
  Dir_Out = 0x010,         // Output direction
  Dir_Unidentified = 0x001 // Unidentified direction
};
```
For instance, in the provided JSON file snippet:

```json
{
  "Direction": {
    "BF_crypt(0)": 256,
    "BF_crypt(1)": 272,
    "BF_crypt(2)": 272,
    "BF_decode(1)": 272
  }
}
```
- The entry **"BF_crypt(0)": 256** indicates that the direction of the first parameter in the BF_crypt function is set to 256 (0x100), signifying an **In** direction, meaning it is used for input.
- Similarly, **"BF_crypt(1)": 272** reveals that the direction of the second parameter in the BF_crypt function is 272 (0x110), indicating it has both **In** and **Out** directions, meaning it is utilized for both input and output.

This notation helps in understanding how each parameter within a function is used, whether for input, output, or both, providing clear insights into the data flow and operations performed by the function.

## ut_analyzer

UT Analyzer analyzes unit test code for a target library and generates a result as json file.
Mandatory command line options are below.

```shell
ut_analyzer --entry ${entry_path} --extern ${extern_path} --ut ${ut_type} --name ${lib_name} --public ${api_json_path} --out ${output_path}
```

Most options are same as the [target_analyzer](#target_analyzer).
Note that, *entry_path* should specify AST/IR files for a unit test executable, not library.

### ut_type

Framework used by the target project, could be `tct`, `gtest`, or `boost`.

## fuzz_generator

fuzz_generator generates fuzz drivers using report files of [target_anlayzer](#target_analyzer)
and [ut_analyzer](#ut_analyzer).
Mandatory command line options are below.

```shell
fuzz_generator --src ${src_path} --target ${target_analyzer_report_path} --ut ${ut_analyzer_report_path} --public ${api_json_path} --out ${output_dir}
```

### src_path

src_path is the directory path where unit test source code is stored. fuzz_generator copy this directory and
generates fuzz driver by modifying these copied files.

Other options are same as the [target_analyzer](#target_analyzer) options.

### How fuzz driver works

This section describes the functionality and implementation details of the generated fuzz driver, which is crucial for understanding how fuzz testing is integrated with the target source code.

**fuzz_entry.cc** (Auto-Generated File)
```c++
DEFINE_PROTO_FUZZER(const AutoFuzz::FuzzArgsProfile &autofuzz_mutation) {
  ... /* Values are assigned from autofuzz_mutation */
  enterAutofuzz();
}
```
This function serves as the entry point for the generated fuzz driver. It takes values generated by the fuzzer and assigns them to variables that are subsequently used to invoke library functions. Finally, the function calls **enterAutofuzz();** to proceed with the fuzz testing process.

**Source Code Defining the Target Test Case**
```c++
#ifdef __cplusplus
extern "C" {
#endif
void enterAutofuzz() {
  class AutofuzzTest : public ::Parser_TestArray_Test {
  public:
    void runTest() {
      try {
        SetUpTestCase();
      } catch (std::exception &E) {}
      try {
        SetUp();
      } catch (std::exception &E) {}
      try {
        TestBody();
      } catch (std::exception &E) {}
      try {
        TearDown();
      } catch (std::exception &E) {}
      try {
        TearDownTestCase();
      } catch (std::exception &E) {}
    }
  };
  AutofuzzTest Fuzzer;
  Fuzzer.runTest();
}
#ifdef __cplusplus
}
#endif
```
In the final part of the source code that defines the target test case, UTopia injects the **enterAutofuzz** function. Within this function, the **AutofuzzTest** class is declared, inheriting from **::Parser_TestArray_Test**. This parent class is defined by the GoogleTest framework and is specific to the test case being addressed, as shown below:

```c++
TEST(Parser, TestArray)
{
  ...
}
```
The **runTest()** method executes the test case independently of GoogleTest, invoking five functions that GoogleTest typically calls for each test case. This approach allows for direct test execution without relying on the GoogleTest framework.

## Build generated fuzz drivers

Fuzz drivers are generated in `${output_dir}` passed to fuzz_generator as command line option.
You can build them using same compiler command for unit test executable.
Note that, you should include `fuzz_entry.cc`, `FuzzArgsProto.pb.cc` files that are generated by fuzz_generator.
You can find those files in `${output_dir}`.

# Reproduce Evaluation

## Generate Fuzzer
```bash
python3 -m helper.make {library name}
python3 -m helper.build {library name}
```
You can find every outputs from the whole pipeline of 'UTopia' under following two directories:
- `exp/{library name}/output`,
- `result/test/{library name}`