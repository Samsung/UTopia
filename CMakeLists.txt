project(fuzz_test_generation)
cmake_minimum_required(VERSION 3.5)
# Reference cmake : https://github.com/nsumner/clang-plugins-demo/CMakeLists.txt

## Please define LLVM_DIR with -D<LLVM_DIR>
# LLVM_DIR must be set to the prefix of /share/llvm/cmake via commandline
if (NOT DEFINED LLVM_DIR)
  message(STATUS "!!!NOT DEFINED LLVM_DIR, USE DEFAULT VALUE")
  set(LLVM_DIR /usr)
endif()
message(STATUS "!!!LLVM_DIR: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/")
list(APPEND CMAKE_MODULE_PATH "${LLVM_DIR}")

set(PACKAGE_NAME fuzz_test_generation)
set(PACKAGE_VERSION 0.1)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type is not set. Using Debug as default.")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++14 -fno-rtti -fno-omit-frame-pointer")
# Compatibility for old protobuf versions
add_compile_options("-DGOOGLE_PROTOBUF_NO_RTTI")

# do coverage binary if -DCMAKE_BUILD_TYPE=DEBUG
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")

# do srtip binary if -DCMAKE_BUILD_TYPE=RELEASE
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")

# change output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

############## LLVM CONFIGURATION #################
find_package(LLVM REQUIRED CONFIG HINTS ${LLVM_DIR})
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(REQ_LLVM_LIBRARIES ${LLVM_TARGETS_TO_BUILD}
  Core Support Irreader Passes Option FrontendOpenMP Coverage ObjCARCOpts Coroutines LTO
)

# ADD the CMake features provided by LLVM:
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

option(LLVM_ENABLE_CXX1Y  "Enable C++1Y" ON)
option(LLVM_INCLUDE_TOOLS "Generate build targets for the LLVM tools." ON)
option(LLVM_BUILD_TOOLS
  "Build the LLVM tools. If OFF, just generate build targets." ON)

find_package(Clang REQUIRED)

message("LLVM STATUS:
  Definitions ${LLVM_DEFINITIONS}
  Includes    ${LLVM_INCLUDE_DIRS}
              ${CLANG_INCLUDE_DIRS}
  Libraries   ${LLVM_LIBRARY_DIRS}
  LLVM_CMAKE_DIR: ${LLVM_CMAKE_DIR}
  AddLLVM: ${LIBRARY_OUTPUT_PATH}")

# SET the LLVM header and library paths:
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

############## FINAL PROJECT CONFIG #################
# External dependencies
add_subdirectory(external)

# And the project header and library paths
include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${LIBRARY_OUTPUT_PATH})

add_subdirectory(lib)
add_subdirectory(tools)

# Unit test
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
