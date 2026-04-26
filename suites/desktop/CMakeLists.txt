cmake_minimum_required(VERSION 3.15)

project(rofi)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)

option(BUILD_SMTRECONFIG "Build SMT-based reconfiguration library." FALSE)
option(BUILD_HEADLESS "Build only parts of the project, that can run headlessly." FALSE)
option(BUILD_GAZEBO "Build parts that require gazebo libraries." TRUE)
option(BUILD_GZ_SIMULATOR "Build simulator plugins and Gazebo worlds." FALSE)
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
  GIT_TAG        v2.13.6
)
FetchContent_GetProperties(pybind11)
if(NOT pybind11_POPULATED)
  FetchContent_Populate(pybind11)
  add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR})
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${pybind11_SOURCE_DIR}/contrib")
endif()

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
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
  GIT_TAG        v3.2.0
)
FetchContent_GetProperties(cxxopts)
if(NOT cxxopts_POPULATED)
  FetchContent_Populate(cxxopts)
  add_subdirectory(${cxxopts_SOURCE_DIR} ${cxxopts_BINARY_DIR})
endif()
set_target_properties(cxxopts PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:cxxopts,INTERFACE_INCLUDE_DIRECTORIES>)

find_package(Armadillo REQUIRED)
find_package(CLP REQUIRED)

if (${BUILD_GAZEBO})
  find_package(gz-transport 15 REQUIRED)
endif()

if (${BUILD_GZ_SIMULATOR})
  find_package(gz-plugin 4 REQUIRED COMPONENTS register)
  find_package(gz-sim 10 REQUIRED)
endif()

if (NOT ${BUILD_HEADLESS})
  find_package(VTK REQUIRED COMPONENTS
      CommonColor
      CommonCore
      CommonDataModel
      FiltersCore
      FiltersGeneral
      FiltersHybrid
      FiltersSources
      GUISupportQt
      IOGeometry
      IOImage
      InteractionStyle
      RenderingAnnotation
      RenderingCore
      RenderingOpenGL2)
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
add_component(voxelSerialization)
add_component(kinematics)
add_component(reconfig)
add_component(shapeReconfig)
add_component(torqueComputation)
if (${BUILD_SMTRECONFIG})
  add_component(smtreconfig)
endif()
add_component(lwip++)
add_component(rofiHalInc)
if (NOT ${BUILD_GAZEBO})
  add_component(rofiHalHeadless)
  add_library(rofi::hal ALIAS rofi_hal_headless)
endif()

# Libraries for simulators
add_component(lwipcore)
# Temporary alias:
add_library(idf::lwip ALIAS lwipcore)
add_component(networking)
add_component(leadership)
add_component(taskDistribution)
add_component(snake_reconfig)

if (${BUILD_GAZEBO})
  add_component(gzTransportCompat)
  add_component(rofisimMessages)
  add_component(messageLogger)

  add_component(messageServer)

  add_component(rofiHalSim)
  add_component(rofiHalSimPy)

  add_component(simplesimConfigMsgs)
  add_component(simplesimLib)
  add_component(simplesimPacketFilters)
  add_component(simplesimClientLib)
  add_component(simplesimServerLib)

  if (${BUILD_GZ_SIMULATOR})
    add_component(gazebosim)
  endif()

  # Introduce alias for the HAL
  add_library(rofi::hal ALIAS rofi_hal_sim)
endif()

if (${BUILD_RUST})
  add_component(voxelReconfig)
endif()


add_tool(convert)
add_tool(generate)
add_tool(emptyDockFill)
add_tool(topology2dot)
add_tool(ik)
add_tool(tangle)
add_tool(torqueCompute)

if (NOT ${BUILD_HEADLESS})
  add_tool(visualizer)
  add_tool(rofiTool)
endif()

if (${BUILD_GAZEBO})
  if (${BUILD_GZ_SIMULATOR})
    add_tool(gazebosimTools)
  endif()
  add_tool(simplesimServer)

  if (NOT ${BUILD_HEADLESS})
    add_tool(simplesimClient)
    add_tool(simplesim)
  endif()
endif()

if (${BUILD_RUST})
  add_tool(voxel)
endif()


if (${BUILD_GAZEBO} AND ${BUILD_GZ_SIMULATOR})
  add_subdirectory($ENV{ROFI_ROOT}/data/gazebo/worlds data/gazebo/worlds)
  add_subdirectory($ENV{ROFI_ROOT}/examples/simulator examples/simulator)
  add_subdirectory($ENV{ROFI_ROOT}/tutorials/simulator tutorials/simulator)
endif()

# Add logic tests for the hardware-related suites
add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/rofiHalEsp32/test-logic rofiHalEsp32-test)


list_targets()
list_dependencies()
list_sources()
