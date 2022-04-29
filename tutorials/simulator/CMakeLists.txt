cmake_minimum_required(VERSION 3.11)

set(SOLUTION_DIRS
    task_1_solution # Used only for configuration generator
    task_2_solution
    task_3_solution
    task_4_solution
    task_5_solution
    task_6_solution
)

foreach(solution_dir ${SOLUTION_DIRS})
    add_subdirectory(${solution_dir})
endforeach()
