<p align="center">
  <img src="res/UTopia.png">
</p>

## Introduction

*UTopia* is a tool for automatically generating fuzz drivers from unit tests.

*UTopia* will let developers perform fuzz testing without special knowledge about writing fuzzers. Even developers
familiar with fuzzing can save significant time by generating fuzz drivers automatically.

*UTopia* supports C/C++ libraries which have unit tests with GoogleTest, Boost.Test or Tizen TCT.

## Trophy

To see bugs found by *UTopia*, visit [Trophy](Trophy.md) page. You can also see some *UTopia*-based fuzzers there.

## Docker

For easy set up, we provide docker image for running *UTopia*.
You can build *UTopia* and generate/run fuzzers inside docker container.

Build docker image with command below.

```shell
docker buildx build -f docker/Dockerfile -t utopia .
```

## Build

*UTopia* depends on LLVM, Protobuf and GoogleTest. You can install dependencies manually, but we recommend to use given
docker image.

To build *UTopia*, follow cmake process below.

```shell
cd $UTOPIA_HOME_DIR
cmake -B build -S .
cmake --build build -j$(nproc)
```

## Run
For some selected projects, you can use helper script to run our tool without further effort. Please
refer [helper/README.md](helper/README.md).
For other projects, please check following manual.

### target_analyzer

Target Analyzer analyzes target library code and generates a result as json file.
Mandatory command line options are below.

```shell
target_analyzer --entry ${entry_path} --extern ${extern_path} --name ${lib_name} --public ${api_json_path} --out ${output_path}
```

#### entry_path

Entry is a json file to indicate paths of AST and IR files of target library code.
Its format looks like below.

library.a.json

```json
{
  "binary_info": {
    "libcommon.a": {
      "bc_file": "/root/fuzz-test-generation/exp/sample/output/bc/libcommon.a.bc"
    }
  },
  "project_dir": "/root/fuzz-test-generation/exp/sample",
  "project_name": "sample"
}
```

You should specify LLVM bitcode file path of a specific library using **"bc_file"** keyword.
We only accept one bitcode file so far, thus you may use `llvm-link` to link several bit code files
to one bit code file.

Note that, if *entry_path* is `library.a.json`, there should be `compiles_library.a.json` in the same directory has the
same format. And its format is like below.

compiles_library.a.json

```json
[
  {
    "ast_file": "/root/fuzz-test-generation/exp/sample/libcommon.a_ast/codec/common/src/ast1.o.ast"
  },
  {
    "ast_file": "/root/fuzz-test-generation/exp/sample/libcommon.a_ast/codec/common/src/ast2.o.ast"
  }
]
```

You should specify AST file path of a specific library using **"ast_file"** keyword and its array.
Note that a specified bitcode file and ast files are generated from same source codes for a specific library.

#### extern_path

Target Analyzer accepts target analyzer report of other libraries for an accurate result.
This path should be a directory path where other reports stored, which means more than one library reports are allowed.

#### lib_name

Library name to be analyzed. This name should be specified in **"binary_info"** of entry json file.

#### api_json_path

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

### ut_analyzer

UT Analyzer analyzes unit test code for a target library and generates a result as json file.
Mandatory command line options are below.

```shell
ut_analyzer --entry ${entry_path} --extern ${extern_path} --ut ${ut_type} --name ${lib_name} --public ${api_json_path} --out ${output_path}
```

Most options are same as the [target_analyzer](#target_analyzer).
Note that, *entry_path* should specify AST/IR files for a unit test executable, not library.

#### ut_type

Framework used by the target project, could be `tct`, `gtest`, or `boost`.

### fuzz_generator

fuzz_generator generates fuzz drivers using report files of [target_anlayzer](#target_analyzer)
and [ut_analyzer](#ut_analyzer).
Mandatory command line options are below.

```shell
fuzz_generator --src ${src_path} --target ${target_analyzer_report_path} --ut ${ut_analyzer_report_path} --public ${api_json_path} --out ${output_dir}
```

#### src_path

src_path is the directory path where unit test source code is stored. fuzz_generator copy this directory and
generates fuzz driver by modifying these copied files.

Other options are same as the [target_analyzer](#target_analyzer) options.

### Build generated fuzz drivers

Fuzz drivers are generated in `${output_dir}` passed to fuzz_generator as command line option.
You can build them using same compiler command for unit test executable.
Note that, you should include `fuzz_entry.cc`, `FuzzArgsProto.pb.cc` files that are generated by fuzz_generator.
You can find those files in `${output_dir}`.
