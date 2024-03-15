cmake_minimum_required(VERSION 3.6)
project(rofi)

add_library(kinematics src/fReconfig.cpp)
target_include_directories(kinematics PUBLIC include)
target_link_libraries(kinematics PUBLIC configuration legacy-configuration atoms geometry)

add_library(planning src/fabrik.cpp src/planning.cpp)
target_include_directories(planning PUBLIC include)
target_link_libraries(planning PUBLIC configurationWithJson legacy-configuration kinematics atoms geometry)

add_library(tree-reconfig src/treeReconfig.cpp src/mcs.cpp)
target_include_directories(tree-reconfig PUBLIC include)
target_link_libraries(tree-reconfig PUBLIC planning isoreconfig rofi::hal)

if(TARGET Catch2WithMain)
  file(GLOB TEST_SRC test/*.cpp)
  add_executable(test-planning ${TEST_SRC})
  target_include_directories(test-planning PUBLIC include)
  target_link_libraries(test-planning PUBLIC Catch2WithMain atoms planning parsing-lite configurationWithJson)
elseif(TARGET Catch2::Catch2)
  message(WARNING "Catch2 available, but not Catch2WithMain. Tests for geometry will not build.")
endif()