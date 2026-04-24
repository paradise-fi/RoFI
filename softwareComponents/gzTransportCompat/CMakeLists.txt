cmake_minimum_required(VERSION 3.11)

add_library(gzTransportCompat INTERFACE)
target_link_libraries(gzTransportCompat INTERFACE gz-transport::gz-transport)
target_include_directories(gzTransportCompat INTERFACE include)
