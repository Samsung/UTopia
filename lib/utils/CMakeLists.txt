add_library(ftgUtils
    ManualAllocLoader.cpp
    ASTUtil.cpp
    BuildDBParser.cpp
    FileUtil.cpp
    StringUtil.cpp
    json/jsoncpp.cpp
    IRUtil.cpp
)

target_link_libraries(ftgUtils
  stdc++fs
  LLVMDemangle
)
