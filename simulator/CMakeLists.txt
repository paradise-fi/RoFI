cmake_minimum_required(VERSION 3.10)
project(rofisim)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

set(SOURCE_CODE_FLAGS "-Wall -Wextra -pedantic")


add_subdirectory(plugins)
add_subdirectory(tools)
