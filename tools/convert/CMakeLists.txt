cmake_minimum_required(VERSION 3.11)

add_executable(rofi-convert main.cpp voxel_json_cmd.cpp)
target_link_libraries(rofi-convert PRIVATE configurationWithJson dimcli nlohmann_json::nlohmann_json magic_enum)
