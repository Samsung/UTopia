SET(SRCS
    Generator.cpp
    Fuzzer.cpp
    FuzzInput.cpp
    GenLoader.cpp
    FuzzGenReporter.cpp
    CorpusGenerator.cpp
    FuzzStatus.cpp
    FuzzerSrcGenerator.cpp
    UTModify.cpp
    ProtobufDescriptor.cpp
    InputMutator.cpp
    ProtobufMutator.cpp
    SrcGenerator.cpp
)

if (DEFINED INCLUDE_NOT_FUZZABLE_TC)
   add_compile_options("-DINCLUDE_NOT_FUZZABLE_TC")
endif()

add_library(ftgGeneration ${SRCS})

target_link_libraries(ftgGeneration
    clangToolingInclusions
)
