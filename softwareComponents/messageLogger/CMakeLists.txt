cmake_minimum_required(VERSION 3.11)


set (FILES
    src/message_logger.cpp
)

add_library(messageLogger INTERFACE)
target_link_libraries(messageLogger INTERFACE fmt)
target_include_directories(messageLogger INTERFACE include)
