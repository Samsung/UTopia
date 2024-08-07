project(fuzz_generator)

add_executable(fuzz_generator main.cpp)

target_link_libraries(fuzz_generator
  -Wl,--start-group
  ftgAnalysis
  ftgASTIRMap
  ftgConstantAnalysis
  ftgGeneration
  ftgInputAnalysis
  ftgInputFilter
  ftgPropAnalysis
  ftgRootDefAnalysis
  ftgSourceAnalysis
  ftgSourceLoader
  ftgTargetAnalysis
  ftgTCAnalysis
  ftgType
  ftgUTAnalysis
  ftgUtils
  ftgAPILoader
  spdlog::spdlog
  clangBasic
  clangFormat
  clangFrontend
  clangCodeGen
  clangSerialization
  clangDriver
  clangParse
  clangSema
  clangAnalysis
  clangAST
  clangASTMatchers
  clangEdit
  clangLex
  clangRewrite
  clangToolingCore
  clangTooling
  ${REQ_LLVM_LIBRARIES}
  -Wl,--end-group
)

if(LLVM_VERSION_MAJOR VERSION_GREATER_EQUAL 17)
target_link_libraries(fuzz_generator
  clangSupport
)
endif()
