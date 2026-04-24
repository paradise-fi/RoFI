cmake_minimum_required(VERSION 3.11)


set (FILES
    src/message_server.cpp
)

add_library(messageServer ${FILES})
target_link_libraries(messageServer PUBLIC gzTransportCompat)
target_include_directories(messageServer PUBLIC include)
