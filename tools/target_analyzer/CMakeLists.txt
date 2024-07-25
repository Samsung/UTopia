project(target_analyzer)

add_executable(target_analyzer main.cpp)

target_link_libraries(target_analyzer
  -Wl,--start-group
  ftgTargetAnalysis
  ftgPropAnalysis
  ftgTargetAnalysis
  ftgType
  ftgSourceLoader
  ftgAPILoader
  ftgAnalysis
  ftgConstantAnalysis
  ftgUtils
  -Wl,--end-group
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

if(LLVM_VERSION_MAJOR VERSION_GREATER_EQUAL 17)
target_link_libraries(target_analyzer
  clangSupport
)
endif()
