cmake_minimum_required(VERSION 3.11)

add_library(rofiPluginCommon INTERFACE)
target_link_libraries(rofiPluginCommon INTERFACE gz-sim::gz-sim gz-plugin::gz-plugin gzTransportCompat ${Boost_LIBRARIES} rofisimMessages)
target_include_directories(rofiPluginCommon INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
