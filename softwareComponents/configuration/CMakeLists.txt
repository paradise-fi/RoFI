cmake_minimum_required(VERSION 3.6)
project(rofi)

find_package(Armadillo REQUIRED)

file(GLOB LEGACY_CONF_SRC legacy/configuration/*.cpp)
add_library(legacy-configuration STATIC ${LEGACY_CONF_SRC})
target_include_directories(legacy-configuration PUBLIC .)
target_link_libraries(legacy-configuration PUBLIC ${ARMADILLO_LIBRARIES})


file(GLOB CONFIGURATION_SRC src/*)
add_library(configuration STATIC ${CONFIGURATION_SRC})
target_include_directories(configuration PUBLIC include)
target_link_libraries(configuration PUBLIC ${ARMADILLO_LIBRARIES} atoms fmt)

add_library(configurationWithJson INTERFACE)
target_include_directories(configurationWithJson INTERFACE include)
target_link_libraries(configurationWithJson INTERFACE configuration nlohmann_json::nlohmann_json)

add_library(generators STATIC legacy/configuration/Generators.cpp legacy/configuration/Generators.h)
target_include_directories(generators PUBLIC .)
target_link_libraries(generators PUBLIC legacy-configuration)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-configuration ${TEST_SRC})
target_link_libraries(test-configuration PRIVATE Catch2::Catch2 configurationWithJson atoms)
target_include_directories(test-configuration PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} configurationWithJson)
