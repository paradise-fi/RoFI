cmake_minimum_required(VERSION 3.11)

add_library(rofiPluginCommon INTERFACE)
target_link_libraries(rofiPluginCommon INTERFACE ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} rofisimMessages)
target_include_directories(rofiPluginCommon INTERFACE ${CMAKE_CURRENT_LIST_DIR})
