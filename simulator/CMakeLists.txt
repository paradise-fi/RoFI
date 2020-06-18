cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(rofisim)

include(FetchContent)

find_package(gazebo REQUIRED)
include_directories(${GAZEBO_INCLUDE_DIRS})
link_directories(${GAZEBO_LIBRARY_DIRS})

FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.11.1
)
FetchContent_GetProperties(catch)
if(NOT catch_POPULATED)
  FetchContent_Populate(catch)
  add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${catch_SOURCE_DIR}/contrib")
  include(Catch)
  include(ParseAndAddCatchTests)
endif(NOT catch_POPULATED)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${GAZEBO_CXX_FLAGS} -Wall -Wextra -pedantic")

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif(${LIBCXX})


add_subdirectory(plugins)
add_subdirectory(hal)
add_subdirectory(tools)

add_subdirectory(worlds)

add_subdirectory(examples)
add_subdirectory(demo)
