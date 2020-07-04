
cmake_minimum_required(VERSION 3.11)

FetchContent_Declare(
  lwip
  GIT_REPOSITORY git@github.com:jajik/lwip.git
)
FetchContent_GetProperties(lwip)
if(NOT lwip_POPULATED)
  FetchContent_Populate(lwip)
  set(LWIP_INCLUDE_DIRS ${lwip_SOURCE_DIR}/src/include)
  add_subdirectory(${lwip_SOURCE_DIR} ${lwip_BINARY_DIR})
  target_include_directories(lwipcore PUBLIC ${LWIP_INCLUDE_DIRS})
  target_include_directories(lwipcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lwip/include)
  target_sources(lwipcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lwip/sys_arch.c)
  set_property(TARGET lwipcore PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()



add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../RoFIDriver/atoms ${CMAKE_CURRENT_BINARY_DIR}/atoms)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../RoFIDriver/lwip++ ${CMAKE_CURRENT_BINARY_DIR}/lwip++)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../RoFIDriver/networking ${CMAKE_CURRENT_BINARY_DIR}/networking)

add_library(rofiHal SHARED rofi_hal.cpp)
target_link_libraries(rofiHal PUBLIC rofiMessages ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} lwipcore)
target_link_libraries(rofiHal PUBLIC atoms lwip++ networking)
target_include_directories(rofiHal INTERFACE ${CMAKE_CURRENT_LIST_DIR})


add_subdirectory(tests)
