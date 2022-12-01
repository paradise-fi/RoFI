cmake_minimum_required(VERSION 3.6)
project(rofi)

add_resources(MODEL_RESOURCES
    "model/body.obj"
    "model/connector.obj"
    "model/shoe.obj"
    "model/point.obj"
)

set(ROFI_TOOL_SRCS
    main.cpp
    common.cpp
    rendering.cpp

    build.cpp
    check.cpp
    points.cpp
    preview.cpp
)

add_executable(rofi-tool ${ROFI_TOOL_SRCS} ${MODEL_RESOURCES})
target_link_libraries(rofi-tool PRIVATE dimcli configuration configurationWithJson isoreconfig ${VTK_LIBRARIES} atoms-heavy)
