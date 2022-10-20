cmake_minimum_required(VERSION 3.11)


set(FILES
    src/py_filter.cpp
)

# We have to compile this as static library with hidden symbols; read:
# - https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes
# - https://github.com/vgc/vgc/issues/11
add_library(simplesimPyFilter STATIC ${FILES})
target_compile_options(simplesimPyFilter PUBLIC -fvisibility=hidden)
target_link_libraries(simplesimPyFilter PUBLIC simplesim pybind11::embed)
target_include_directories(simplesimPyFilter PUBLIC include_py)


file(GLOB TEST_SRC test/*.cpp)
add_executable(test-simplesimPyFilter ${TEST_SRC})
target_link_libraries(test-simplesimPyFilter PRIVATE Catch2WithMain simplesimPyFilter)
