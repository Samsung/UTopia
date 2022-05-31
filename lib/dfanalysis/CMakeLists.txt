PROJECT(ftgDFAnalysis)

include_directories(
  ${CMAKE_SOURCE_DIR}/include/dfanalysis
)

add_library(ftgDFAnalysis
  DFTarget.cpp
  DFNode.cpp
  DFBasicTypes.cpp
)
