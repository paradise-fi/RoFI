cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(networking)

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

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../RoFISim/hal/messages ${CMAKE_CURRENT_BINARY_DIR}/messages)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../RoFISim/hal/jthread ${CMAKE_CURRENT_BINARY_DIR}/jthread)


add_subdirectory(gazebo_master)
add_subdirectory(rofi_interface)
add_subdirectory(simulation)
add_subdirectory(controllers)

set (FILES
    main.cpp
)

add_executable(master ${FILES})
target_link_libraries(master PUBLIC networkingGazeboMaster)
target_link_libraries(master PUBLIC networkingRofiInterface)
target_link_libraries(master PUBLIC networkingSimulation)
target_link_libraries(master PUBLIC networkingControllers)
