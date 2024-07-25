project(tests)

include_directories(
  ${CMAKE_SOURCE_DIR}/tests
)

add_executable(tests
    GtestMain.cpp
    TestHelper.cpp
    testutil/APIManualLoader.cpp
    testutil/CompileHelper.cpp
    testutil/IRAccessHelper.cpp
    testutil/SourceFileManager.cpp
    testutil/TestUtil.cpp

    TestASTUtils.cpp

    astirmap/TestDebugInfoMap.cpp
    astirmap/TestIRNode.cpp
    astirmap/TestMacroMapper.cpp
    astirmap/TestMatchers.cpp
    astirmap/TestLocIndex.cpp

    utanalysis/TestTargetAPIFinder.cpp

    analysis/TestTypeAnalysisReport.cpp
    analysis/TestTypeAnalyzer.cpp

    astvalue/TestASTValue.cpp

    generation/TestFTG.cpp
    generation/TestFuzzInput.cpp
    generation/TestUTModify.cpp

    inputanalysis/TestDefAnalyzer.cpp
    inputanalysis/TestDefMapGenerator.cpp
    inputanalysis/TestInputAnalysisReport.cpp

    utils/TestAssignUtil.cpp
    utils/TestStringUtil.cpp
    utils/TestFileUtil.cpp
    utils/TestLLVMUtil.cpp

    apiloader/TestAPILoader.cpp

    sourceanalysis/TestSourceAnalyzerImpl.cpp
    sourceloader/TestBuildDB.cpp
    sourceloader/TestBuildDBLoader.cpp
    constantanalysis/TestConstAnalyzer.cpp
    constantanalysis/TestConstAnalyzerReport.cpp
    propanalysis/TestAllocSizeAnalysisReport.cpp
    propanalysis/TestAllocSizeAnalyzer.cpp
    propanalysis/TestArrayAnalysisReport.cpp
    propanalysis/TestArrayAnalyzer.cpp
    propanalysis/TestDirectionAnalysisReport.cpp
    propanalysis/TestDirectionAnalyzer.cpp
    propanalysis/TestFilePathAnalysisReport.cpp
    propanalysis/TestFilePathAnalyzer.cpp
    propanalysis/TestLoopAnalyzer.cpp

    indcallsolver/TestIndCallSolver.cpp
    inputfilter/TestInputFilter.cpp

    tcanalysis/TestGoogletestExtractor.cpp
    tcanalysis/TestTCTExtractor.cpp
    tcanalysis/TestGoogleTestCallWriter.cpp
    tcanalysis/TestTCTCallWriter.cpp
    tcanalysis/TestTCAnalyzerFactory.cpp

    targetanalysis/TestTargetLib.cpp

    rootdefanalysis/TestRDSpace.cpp
    rootdefanalysis/TestRootDef.cpp
    rootdefanalysis/TestField.cpp
    rootdefanalysis/TestClass.cpp
    type/TestType.cpp
    type/TestGlobalDef.cpp
    type/TestArrayInfo.cpp
    generation/TestFuzzStatus.cpp
    generation/TestProtobufDescriptor.cpp
    generation/TestProtobufMutator.cpp)

find_package(Boost
    COMPONENTS unit_test_framework
    )
if (Boost_FOUND)
  message(STATUS "Found Boost : Enable Boost test")
  target_sources(tests PRIVATE
    tcanalysis/TestBoostExtractor.cpp
    tcanalysis/TestBoostCallWriter.cpp
  )
endif()

target_include_directories(tests
    PRIVATE ${CMAKE_SOURCE_DIR}/external/approvaltests
)

target_link_libraries(tests
  -Wl,--start-group
  ftgAnalysis
  ftgAPILoader
  ftgASTIRMap
  ftgConstantAnalysis
  ftgGeneration
  ftgInputAnalysis
  ftgInputFilter
  ftgICallSolver
  ftgPropAnalysis
  ftgRootDefAnalysis
  ftgSourceAnalysis
  ftgSourceLoader
  ftgTargetAnalysis
  ftgTCAnalysis
  ftgType
  ftgUTAnalysis
  ftgUtils
  gtest
  libprotobuf.a
  libprotoc.a
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
  pthread
  -Wl,--end-group
)

add_custom_target(link_resources ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
          ${CMAKE_CURRENT_SOURCE_DIR}/resources
          ${CMAKE_CURRENT_BINARY_DIR}/resources
)

include(GoogleTest)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.10)
  gtest_discover_tests(tests
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/ut_result
)
else() # Tizen 5.5
  gtest_add_tests(TARGET tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

if(LLVM_VERSION_MAJOR VERSION_GREATER_EQUAL 17)
target_link_libraries(tests
  clangSupport
)
endif()
