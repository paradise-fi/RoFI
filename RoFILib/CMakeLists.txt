cmake_minimum_required(VERSION 3.11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../releng/cmake")

project(rofi)

include(FetchContent)

# Build options
option(VISUALIZER "Build visualizer" ON)
option(DISTRIBUTED_RECONFIG "Build distributed reconfiguraiton tool" OFF)
option(SMT_RECONFIG "Build SMT reconfiguration" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

find_package(Armadillo REQUIRED)

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
endif()

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        6.1.2
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()

FetchContent_Declare(
  cxxopts
  GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
  GIT_TAG        v2.2.0
)
FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
  FetchContent_Populate(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR})
endif()

add_subdirectory(atoms)
add_subdirectory(configuration)
if(DISTRIBUTED_RECONFIG)
  add_subdirectory(distribute)
endif()
add_subdirectory(generate)
add_subdirectory(reconfig)
if(VISUALIZER)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
  add_subdirectory(visualizer)
endif()
add_subdirectory(util)
if(SMT_RECONFIG)
  add_subdirectory(smtreconfig)
endif()
add_subdirectory(snake_reconfig)
