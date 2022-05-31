string(REPLACE " -Wall -Werror" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

add_subdirectory(spdlog)
