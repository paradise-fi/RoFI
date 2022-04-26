cmake_minimum_required(VERSION 3.11)

set(SOLUTION_DIRS
)

foreach(solution_dir ${SOLUTION_DIRS})
    add_subdirectory(${solution_dir})
endforeach()
