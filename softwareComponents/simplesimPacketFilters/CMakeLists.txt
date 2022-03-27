cmake_minimum_required(VERSION 3.11)


set(FILES
    src/py_filter.cpp
)

add_library(simplesimPyFilter ${FILES})
target_link_libraries(simplesimPyFilter PUBLIC simplesim pybind11::embed)
target_include_directories(simplesimPyFilter PUBLIC include_py)


file(GLOB TEST_SRC test/*.cpp)
add_executable(test-simplesimPyFilter ${TEST_SRC})
target_link_libraries(test-simplesimPyFilter PRIVATE Catch2WithMain simplesimPyFilter)
