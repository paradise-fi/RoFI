cmake_minimum_required(VERSION 3.12)

project(rofi)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
option(BUILD_SMTRECONFIG "Build SMT-based reconfiguration library." FALSE)
option(BUILD_HEADLESS "Build only parts of the project, that can run headlessly." FALSE)
option(BUILD_GAZEBO "Build parts that require gazebo libraries." TRUE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

include(FetchContent)
include(CollectTargets)

function(add_component componentName)
    add_subdirectory($ENV{ROFI_ROOT}/softwareComponents/${componentName} ${componentName})
endfunction()

function(add_tool toolName)
    add_subdirectory($ENV{ROFI_ROOT}/tools/${toolName} ${toolName})
endfunction()


if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color=always)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

if(${LIBCXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
endif(${LIBCXX})

# Specify common dependencies
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
  dimcli
  URL https://github.com/gknowles/dimcli/archive/refs/tags/v6.0.0.zip
)
FetchContent_GetProperties(dimcli)
if(NOT dimcli_POPULATED)
  FetchContent_Populate(dimcli)
  add_library(dimcli STATIC ${dimcli_SOURCE_DIR}/libs/dimcli/cli.cpp)
  target_include_directories(dimcli PUBLIC ${dimcli_SOURCE_DIR}/libs)
endif()

if (${BUILD_GAZEBO})
  find_package(gazebo REQUIRED)
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



add_component(atoms)
add_component(configuration)
add_component(kinematics)
add_component(reconfig)
if (${BUILD_SMTRECONFIG})
  add_component(smtreconfig)
endif()
add_component(lwip++)

# Libraries for simulators
add_component(lwipcore)
add_component(networking)
add_component(snake_reconfig)

if (${BUILD_GAZEBO})
  add_component(rofisimMessages)
  add_component(configurationMessages)
  add_component(rofiHalSim)
  add_component(rofiHalSimPy)
  add_component(gazebosim)
  add_component(simplesim)
endif()


add_tool(generate)
add_tool(emptyDockFill)
add_tool(topology2dot)
add_tool(ik)
add_tool(rofiTool)
add_tool(tangle)

if (NOT ${BUILD_HEADLESS})
  add_tool(visualizer)
endif()

if (${BUILD_GAZEBO})
  add_tool(gazebosimTools)
endif()


if (${BUILD_GAZEBO})
  add_subdirectory($ENV{ROFI_ROOT}/data/gazebo/worlds data/gazebo/worlds)
  add_subdirectory($ENV{ROFI_ROOT}/examples/simulator examples/simulator)
endif()


list_targets()
