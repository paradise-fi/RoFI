cmake_minimum_required(VERSION 3.6)
project(rofi)


file(GLOB LEGACY_CONF_SRC legacy_src/*)
add_library(legacy-configuration STATIC ${LEGACY_CONF_SRC})
target_include_directories(legacy-configuration PUBLIC legacy_include combined_include)
target_link_libraries(legacy-configuration PUBLIC armadillo)


file(GLOB CONFIGURATION_SRC src/*)
add_library(configuration STATIC ${CONFIGURATION_SRC})
target_include_directories(configuration PUBLIC include combined_include/legacy)
target_link_libraries(configuration PUBLIC armadillo atoms fmt)

add_library(configurationWithJson INTERFACE)
target_include_directories(configurationWithJson INTERFACE json_include)
target_link_libraries(configurationWithJson INTERFACE configuration nlohmann_json::nlohmann_json)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-configuration ${TEST_SRC})
target_link_libraries(test-configuration PRIVATE Catch2WithMain configurationWithJson atoms)
