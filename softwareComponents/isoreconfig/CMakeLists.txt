cmake_minimum_required(VERSION 3.6)
project(rofi)


file(GLOB ISOMORPHIC_SRC src/*)
add_library(isoreconfig STATIC ${ISOMORPHIC_SRC})
target_include_directories(isoreconfig PUBLIC include)
target_link_libraries(isoreconfig PUBLIC ${ARMADILLO_LIBRARIES} configuration)
