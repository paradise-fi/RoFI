cmake_minimum_required(VERSION 3.6)
project(rofi)

# Taken from https://beesbuzz.biz/code/4399-Embedding-binary-resources-with-CMake-and-C-11
# Compilation step for static resources
function(add_resources out_var)
  SET(result)
  foreach(in_f ${ARGN})
    set(out_f "${PROJECT_BINARY_DIR}/${in_f}.o")
    get_filename_component(dir ${out_f} DIRECTORY)
    file(MAKE_DIRECTORY ${dir})
    add_custom_command(OUTPUT ${out_f}
      COMMAND ld -r -b binary -o ${out_f} ${in_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Building resource object ${out_f}"
      VERBATIM
      )
    list(APPEND result ${out_f})
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE SRC src/*.cpp)

add_library(atoms INTERFACE)
target_include_directories(atoms INTERFACE include)

add_library(atoms-heavy INTERFACE)
target_include_directories(atoms-heavy INTERFACE include)
target_link_libraries(atoms-heavy INTERFACE "stdc++fs")

# We use C++ tmpname - which triggers warning
target_link_options(atoms-heavy INTERFACE -Wno-deprecated-declarations)

if(TARGET Catch2::Catch2)
  file(GLOB TEST_SRC test/*.cpp)
  add_executable(test-atoms ${TEST_SRC})
  target_link_libraries(test-atoms PRIVATE Catch2::Catch2 atoms atoms-heavy)
endif()
