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

add_library(ftgGeneration ${SRCS})

target_link_libraries(ftgGeneration
    clangToolingInclusions
)
