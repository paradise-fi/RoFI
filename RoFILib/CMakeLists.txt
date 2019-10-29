cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../releng/cmake")

project(rofi)

set(CMAKE_CXX_STANDARD 17)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

find_package(Armadillo REQUIRED)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_package(Catch2 REQUIRED)
include(CTest)
include(Catch)

add_subdirectory("extlib/fmt-6.0.0")



add_subdirectory(configuration)
add_subdirectory(distribute)
add_subdirectory(generate)
add_subdirectory(reconfig)
add_subdirectory(visualizer)
add_subdirectory(util)
add_subdirectory(smtreconfig)
