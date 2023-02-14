cmake_minimum_required(VERSION 3.15)

project(rofi)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)

option(BUILD_SMTRECONFIG "Build SMT-based reconfiguration library." FALSE)
option(BUILD_HEADLESS "Build only parts of the project, that can run headlessly." FALSE)
option(BUILD_GAZEBO "Build parts that require gazebo libraries." TRUE)
option(BUILD_RUST "Build parts that contain or depend on Rust." TRUE)
include(ColoredOutput)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

include(FetchContent)
include(CollectTargets)
include(RmakeUtils)

if (${BUILD_RUST})
  include(RmakeRust)
endif()

function(add_component componentName)
    add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/${componentName} ${componentName})
endfunction()

function(add_tool toolName)
    add_subdirectory($ENV{ROFI_ROOT}/tools/${toolName} ${toolName})
endfunction()

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})


# Specify common dependencies
FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.8
)
FetchContent_GetProperties(catch)
if(NOT catch_POPULATED)
  FetchContent_Populate(catch)
  add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${catch_SOURCE_DIR}/contrib")
  include(Catch)
  include(ParseAndAddCatchTests)
endif()
# Adds `Catch2WithMain` target from Catch v3.x and removes warnings about unused variables
function(add_catch2_with_main)
  set(TEST_CXX_COMPILE_FLAGS -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable)
  set(CATCH2_MAIN_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/catch2_main.cpp)
  file(GENERATE OUTPUT ${CATCH2_MAIN_SOURCE}
      CONTENT "#define CATCH_CONFIG_MAIN\n#include <catch2/catch.hpp>\n")
  add_library(Catch2WithMain ${CATCH2_MAIN_SOURCE})
  target_link_libraries(Catch2WithMain PUBLIC Catch2::Catch2)
  target_compile_options(Catch2WithMain PUBLIC ${TEST_CXX_COMPILE_FLAGS})
endfunction()
add_catch2_with_main()

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        8.1.1
)
FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()

FetchContent_Declare(
  pybind11
  GIT_REPOSITORY https://github.com/pybind/pybind11.git
  GIT_TAG        v2.6.2
)
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${pybind11_SOURCE_DIR}/contrib")
endif()

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent.git
  GIT_TAG        v3.10.2
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
  FetchContent_Populate(json)
  add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR})
endif()

FetchContent_Declare(
  dimcli
  URL https://github.com/gknowles/dimcli/archive/refs/tags/v6.0.0.zip
)
FetchContent_GetProperties(dimcli)
if(NOT dimcli_POPULATED)
  FetchContent_Populate(dimcli)
  add_library(dimcli STATIC ${dimcli_SOURCE_DIR}/libs/dimcli/cli.cpp)
  target_include_directories(dimcli SYSTEM PUBLIC ${dimcli_SOURCE_DIR}/libs)
  target_compile_options(dimcli PRIVATE -w)
endif()

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG        v0.8.1
)
FetchContent_GetProperties(magic_enum)
if(NOT magic_enum_POPULATED)
  FetchContent_Populate(magic_enum)
  add_subdirectory(${magic_enum_SOURCE_DIR} ${magic_enum_BINARY_DIR})
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
set_target_properties(cxxopts PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)

find_package(Armadillo REQUIRED)

if (${BUILD_GAZEBO})
  find_package(gazebo REQUIRED)

  if (${gazebo_VERSION_MAJOR} LESS 11)
    message(WARNING "Old version of gazebo detected")
  endif()

  # Workaround for bug in gazebo that causes problems on `g++11` with `-std=c++20`
  set(GAZEBO_INCLUDE_DIRS "$ENV{ROFI_ROOT}/gazebo_include" ${GAZEBO_INCLUDE_DIRS})

  find_library(LibTBB NAMES tbb)
  link_libraries(gazebo ${LibTBB})

  find_package(Protobuf REQUIRED)
  # Include Gazebo messages
  set(PROTOBUF_IMPORT_DIRS)
  foreach(ITR ${GAZEBO_INCLUDE_DIRS})
      if(ITR MATCHES ".*gazebo-[0-9.]+$")
          set(PROTOBUF_IMPORT_DIRS "${ITR}/gazebo/msgs/proto")
      endif()
  endforeach()
endif()

if (NOT ${BUILD_HEADLESS})
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
  # Silences warnings from VTK headers
  include_directories(SYSTEM ${VTK_INCLUDE_DIRS})
endif()


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wcast-align -Wcast-qual -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wdelete-non-virtual-dtor -Wold-style-cast -Wundef")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -pedantic")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow=compatible-local -Wstrict-null-sentinel -Wlogical-op")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow-uncaptured-local")
endif()



add_component(atoms)
add_component(parsing)
add_component(configuration)
add_component(geometry)
add_component(voxel)
add_component(kinematics)
add_component(reconfig)
add_component(isoreconfig)
if (${BUILD_SMTRECONFIG})
  add_component(smtreconfig)
endif()
add_component(lwip++)

# Libraries for simulators
add_component(lwipcore)
# Temporary alias:
add_library(idf::lwip ALIAS lwipcore)
add_component(networking)
add_component(snake_reconfig)

if (${BUILD_GAZEBO})
  add_component(rofisimMessages)
  add_component(messageLogger)

  add_component(messageServer)

  add_component(rofiHalInc)
  add_component(rofiHalSim)
  add_component(rofiHalSimPy)

  add_component(gazebosim)
  add_component(simplesimConfigMsgs)
  add_component(simplesimLib)
  add_component(simplesimPacketFilters)
  add_component(simplesimClientLib)
  add_component(simplesimServerLib)

  # Introduce alias for the HAL
  add_library(rofi::hal ALIAS rofi_hal_sim)
endif()

if (${BUILD_RUST})
  add_component(rust-rofi_voxel)
endif()


add_tool(convert)
add_tool(generate)
add_tool(emptyDockFill)
add_tool(topology2dot)
add_tool(ik)
add_tool(tangle)

if (NOT ${BUILD_HEADLESS})
  add_tool(visualizer)
  add_tool(rofiTool)
endif()

if (${BUILD_GAZEBO})
  add_tool(gazebosimTools)
  add_tool(simplesimServer)

  if (NOT ${BUILD_HEADLESS})
    add_tool(simplesimClient)
    add_tool(simplesim)
  endif()
endif()

if (${BUILD_RUST})
  add_tool(rust-rofi_voxel_reconfig)
endif()


if (${BUILD_GAZEBO})
  add_subdirectory($ENV{ROFI_ROOT}/data/gazebo/worlds data/gazebo/worlds)
  add_subdirectory($ENV{ROFI_ROOT}/examples/simulator examples/simulator)
  add_subdirectory($ENV{ROFI_ROOT}/tutorials/simulator tutorials/simulator)
endif()

# Add logic tests for the hardware-related suites
add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/rofiHalEsp32/test-logic rofiHalEsp32-test)


list_targets()
list_dependencies()
list_sources()

