cmake_minimum_required(VERSION 3.6)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_executable(rofi-reconfig main.cpp)
target_link_libraries(rofi-reconfig configuration cxxopts)

add_library(reconfig INTERFACE)
target_include_directories(reconfig INTERFACE .)
target_link_libraries(reconfig INTERFACE configuration cxxopts)

add_executable(reconfig-test test/main.cpp test/test.cpp)
target_link_libraries(reconfig-test configuration reconfig Catch2::Catch2)
target_include_directories(reconfig-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
catch_discover_tests(reconfig-test)
