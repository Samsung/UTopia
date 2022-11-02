include_directories(
  ${CMAKE_SOURCE_DIR}/include/ftg/analysis
)

add_library(ftgAnalysis
  ParamNumberAnalysisReport.cpp
  ParamNumberAnalyzer.cpp
  TypeAnalysisReport.cpp
  TypeAnalyzer.cpp
)
