include_directories(
  ${CMAKE_SOURCE_DIR}/include/ftg/rootdefanalysis
)

add_library(ftgRootDefAnalysis
  RDAnalyzer.cpp
  RDBasicTypes.cpp
  RDCache.cpp
  RDExtension.cpp
  RDNode.cpp
  RDSpace.cpp
  RDTarget.cpp
)
