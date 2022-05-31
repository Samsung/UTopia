include_directories(
  ${CMAKE_SOURCE_DIR}/include/ftg/inputanalysis
)

add_library(ftgInputAnalysis
  DefAnalyzer.cpp
  Definition.cpp
  DefMapGenerator.cpp
  InputAnalysisReport.cpp
  InputAnalyzer.cpp
)
