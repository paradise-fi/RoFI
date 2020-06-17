cmake_minimum_required(VERSION 3.11)


set(EMPTY_WORLD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../worlds/empty.world)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../RoFILib/configuration ${CMAKE_CURRENT_BINARY_DIR}/configuration)


add_library(worldCreatorLib STATIC worldCreator.cpp)
target_link_libraries(worldCreatorLib configuration rofiPluginCommon ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} armadillo)

add_executable(worldCreator worldCreatorMain.cpp)
target_link_libraries(worldCreator worldCreatorLib)
target_compile_definitions(worldCreator PUBLIC WORLD_FILE=${EMPTY_WORLD_FILE})
