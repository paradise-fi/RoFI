cmake_minimum_required(VERSION 3.11)

if(ESP32_BUILD)
    idf_component_get_property(lwipcore lwip COMPONENT_LIB)
    target_sources(${lwipcore} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/forwarding_table.c)
    target_include_directories(${lwipcore} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)

    set(DUMMY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/dummy_source.c)
    file(GENERATE OUTPUT ${DUMMY_SOURCE} CONTENT "void main() {}\n")
    add_library(lwipcore ${DUMMY_SOURCE})

else()
    FetchContent_Declare(lwip
        GIT_REPOSITORY https://github.com/lwip-tcpip/lwip.git
    )
    FetchContent_GetProperties(lwip)
    if(NOT lwip_POPULATED)
        FetchContent_Populate(lwip)
    endif()

    set(LWIP_DIR ${lwip_SOURCE_DIR})
    set(LWIP_INCLUDE_DIRS ${lwip_SOURCE_DIR}/src/include ${CMAKE_CURRENT_SOURCE_DIR}/include)
    include(${LWIP_DIR}/src/Filelists.cmake)


    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/fwtable/forwarding_table.h ${LWIP_INCLUDE_DIRS}/lwip/forwarding_table.h COPYONLY)
    set(lwipcore_SRCS ${lwipcore_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/forwarding_table.c)

    target_include_directories(lwipcore PUBLIC ${LWIP_INCLUDE_DIRS})
    target_include_directories(lwipcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_sources(lwipcore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sys_arch.c)
    target_sources(lwipcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/forwarding_table.c)

endif()
