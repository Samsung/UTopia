include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include/ftg/targetanalysis)

add_library(ftgTargetAnalysis
  TargetLib.cpp
  TargetLibAnalyzer.cpp
)

target_link_libraries(ftgTargetAnalysis
  ftgICallSolver
  ftgSourceLoader
  libprotoc.a
  libprotobuf.a
)
