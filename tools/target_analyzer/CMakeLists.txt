project(target_analyzer)

add_executable(target_analyzer main.cpp)

target_link_libraries(target_analyzer
  ftgTargetAnalysis
  ftgPropAnalysis
  ftgTargetAnalysis
  ftgType
  ftgSourceLoader
  ftgAPILoader
  ftgAnalysis
  ftgConstantAnalysis
  ftgUtils
)

target_link_libraries(target_analyzer
  -Wl,--start-group
  clangBasic
  clangFrontend
  clangFormat
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
