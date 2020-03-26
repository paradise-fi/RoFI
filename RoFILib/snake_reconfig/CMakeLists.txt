cmake_minimum_required(VERSION 3.10)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_executable(snakeReconfig main.cpp)
target_link_libraries(snakeReconfig PUBLIC configuration generators)

add_executable(snakeReconfig-test test/main.cpp test/test.cpp)
target_include_directories(snakeReconfig-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../extlib/)
