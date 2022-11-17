cmake_minimum_required(VERSION 3.11)

set(CONVERT_SRC
    main.cpp
    voxel_json_cmd.cpp
    )

add_executable(rofi-convert ${CONVERT_SRC})
target_link_libraries(rofi-convert PRIVATE configurationWithJson dimcli nlohmann_json::nlohmann_json magic_enum)

file(GLOB TEST_SRC test/*.cpp)
add_executable(test-convert ${TEST_SRC})
target_link_libraries(test-convert PRIVATE Catch2WithMain configurationWithJson dimcli fmt nlohmann_json::nlohmann_json magic_enum)
target_include_directories(test-convert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
