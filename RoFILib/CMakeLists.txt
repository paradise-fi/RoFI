cmake_minimum_required(VERSION 3.13)
project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

find_package(Armadillo REQUIRED)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})


add_subdirectory(configuration)
add_subdirectory(distribute)
add_subdirectory(generate)
add_subdirectory(reconfig)
add_subdirectory(visualizer)