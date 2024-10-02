cmake_minimum_required(VERSION 3.11)
project(rofi)


file(GLOB TORQUE_COMPUTATION_SRC src/*)
add_library(torqueComputation SHARED ${TORQUE_COMPUTATION_SRC})
target_include_directories(torqueComputation PUBLIC include ${CLP_INCLUDE_DIRS})
target_link_libraries(torqueComputation PUBLIC configuration atoms nlohmann_json::nlohmann_json ${ARMADILLO_LIBRARIES} ${CLP_LIBRARIES})


file(GLOB TEST_SRC test/*.cpp)
add_executable(test-torqueComputation ${TEST_SRC})
target_link_libraries(test-torqueComputation PRIVATE torqueComputation Catch2WithMain)
