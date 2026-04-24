cmake_minimum_required(VERSION 3.6)
project(rofidriver)

file(GLOB SRC src/*.cpp)

add_library(networking STATIC ${SRC})
target_link_libraries(networking PUBLIC atoms lwip++ rofi::hal)
target_include_directories(networking PUBLIC include)

if(ESP32_BUILD)
add_library(networking_jig STATIC ${SRC})
target_link_libraries(networking_jig PUBLIC atoms lwip++ rofi::hal::esp32_jig)
target_include_directories(networking_jig PUBLIC include)
endif()

if(NOT ESP32_BUILD)
add_compile_definitions(networking ROFI_DHCP=1)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-networking ${TEST_SRC})
target_link_libraries(test-networking PRIVATE Catch2WithMain networking)
endif()
