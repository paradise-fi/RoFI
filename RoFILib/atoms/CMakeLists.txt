cmake_minimum_required(VERSION 3.6)
project(rofi)

# Taken from https://beesbuzz.biz/code/4399-Embedding-binary-resources-with-CMake-and-C-11
# Compilation step for static resources
function(add_resources out_var)
  SET(result)
  foreach(in_f ${ARGN})
    file(RELATIVE_PATH src_f ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_f})
    set(out_f "${PROJECT_BINARY_DIR}/${in_f}.o")
    get_filename_component(dir ${out_f} DIRECTORY)
    file(MAKE_DIRECTORY ${dir})
    add_custom_command(OUTPUT ${out_f}
      COMMAND ld -r -b binary -o ${out_f} ${src_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building resource object ${out_f}"
      VERBATIM
      )
    list(APPEND result ${out_f})
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

set(CMAKE_CXX_STANDARD 17)
file(GLOB_RECURSE SRC *.cpp)

add_library(atoms INTERFACE ${SRC})
target_include_directories(atoms INTERFACE)
target_include_directories(atoms INTERFACE .)
target_link_libraries(atoms INTERFACE "stdc++fs")

# We use C++ tmpname - which triggers warning
target_link_options(atoms INTERFACE -Wno-deprecated-declarations)
