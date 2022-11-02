add_library(jsoncpp
    jsoncpp.cpp
)

target_include_directories(jsoncpp
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
