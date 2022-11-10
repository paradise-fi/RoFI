cmake_minimum_required(VERSION 3.11)

set(CONVERT_SRC
    main.cpp
    voxel_json_cmd.cpp
    )

add_executable(rofi-convert ${CONVERT_SRC})
target_link_libraries(rofi-convert PRIVATE configurationWithJson dimcli nlohmann_json::nlohmann_json magic_enum)
