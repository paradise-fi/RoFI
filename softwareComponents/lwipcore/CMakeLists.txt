
cmake_minimum_required(VERSION 3.11)


FetchContent_Declare(
  lwip
  GIT_REPOSITORY https://github.com/jajik/lwip.git
)
FetchContent_GetProperties(lwip)
if(NOT lwip_POPULATED)
  FetchContent_Populate(lwip)
  set(LWIP_INCLUDE_DIRS ${lwip_SOURCE_DIR}/src/include)
  add_subdirectory(${lwip_SOURCE_DIR} ${lwip_BINARY_DIR})
  target_include_directories(lwipcore PUBLIC ${LWIP_INCLUDE_DIRS})
  target_include_directories(lwipcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_sources(lwipcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sys_arch.c)
  set_property(TARGET lwipcore PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
