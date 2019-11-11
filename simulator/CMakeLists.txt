cmake_minimum_required(VERSION 3.10)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

add_subdirectory(plugins/messages)
add_subdirectory(plugins/universalModule)
add_subdirectory(tools)
