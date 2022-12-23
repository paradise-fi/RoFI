cmake_minimum_required(VERSION 3.11)

set(CONVERT_SRC
    main.cpp
    voxel_json.cpp
    )

add_executable(rofi-convert ${CONVERT_SRC})
target_link_libraries(rofi-convert PRIVATE atoms configurationWithJson dimcli voxel)
