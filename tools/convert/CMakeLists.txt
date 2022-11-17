cmake_minimum_required(VERSION 3.11)

set(CONVERT_SRC
    main.cpp
    voxel_json_cmd.cpp
    )

add_executable(rofi-convert ${CONVERT_SRC})
target_link_libraries(rofi-convert PRIVATE atoms configurationWithJson dimcli voxel)
