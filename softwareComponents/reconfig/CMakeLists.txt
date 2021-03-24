cmake_minimum_required(VERSION 3.6)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_executable(rofi-reconfig main.cpp)
target_link_libraries(rofi-reconfig PUBLIC reconfig configuration generators cxxopts)

add_library(reconfig STATIC Algorithms.h astar.cpp bfs.cpp rrt.cpp)
target_include_directories(reconfig INTERFACE .)
target_link_libraries(reconfig PUBLIC configuration generators cxxopts)

add_executable(test-reconfig test/main.cpp test/test.cpp)
target_link_libraries(test-reconfig PUBLIC configuration reconfig generators Catch2::Catch2)
target_include_directories(test-reconfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})