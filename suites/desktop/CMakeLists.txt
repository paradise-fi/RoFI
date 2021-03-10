cmake_minimum_required(VERSION 3.12)

project(rofi)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." FALSE)
option(BUILD_SMTRECONFIG "Build SMT-based reconfiguration library." FALSE)
option(BUILD_HEADLESS "Build only parts of the project, that can run headlessly" FALSE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} $ENV{ROFI_ROOT}/releng/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

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



add_component(atoms)
add_component(configuration)
# add_component(kinematics)
add_component(reconfig)
if (${BUILD_SMTRECONFIG})
  add_component(smtreconfig)
endif()

if (NOT ${BUILD_HEADLESS})
  add_tool(visualizer)
endif()

add_tool(generate)
add_tool(emptyDockFill)
add_tool(topology2dot)

list_targets()
