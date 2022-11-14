add_library(ftgUtils
    ManualAllocLoader.cpp
    ASTUtil.cpp
    FileUtil.cpp
    StringUtil.cpp
    IRUtil.cpp
    Location.cpp
)

target_link_libraries(ftgUtils
  stdc++fs
  LLVMDemangle
)
