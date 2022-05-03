cmake_minimum_required(VERSION 3.11)


set(FILES
    src/simplesim_server.cpp
)

add_library(simplesimServer ${FILES})
target_link_libraries(simplesimServer PUBLIC configurationWithJson simplesimPyFilter dimcli)
target_include_directories(simplesimServer PUBLIC include)
