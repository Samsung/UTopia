include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include/ftg/targetanalysis)
add_subdirectory(targetpb)

add_library(ftgTargetAnalysis
  TargetLib.cpp
  TargetLibAnalyzer.cpp
  TargetLibExportUtil.cpp
  TargetLibLoadUtil.cpp
)

target_link_libraries(ftgTargetAnalysis
  ftgICallSolver
  ftgSourceLoader
  libprotoc.a
  libprotobuf.a
  proto
)
