cmake_minimum_required(VERSION 3.11)

if("${TARGET}" STREQUAL "esp32")
    message(FATAL_ERROR "The logic-tests are not intended to be compiled for ESP32")
endif()

add_executable(test-rofiHalEsp32 testPacket.cpp)
target_link_libraries(test-rofiHalEsp32 Catch2WithMain)
target_include_directories(test-rofiHalEsp32 PRIVATE ../include)
