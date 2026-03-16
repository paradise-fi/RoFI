cmake_minimum_required(VERSION 3.6)
project(rofi)

add_resources(MODEL_RESOURCES
    "model/body.obj"
    "model/connector.obj"
    "model/shoe.obj"
    "model/point.obj"
    "model/cube.obj"
)

set(ROFI_TOOL_SRCS
    main.cpp
    rendering.cpp

    build.cpp
    check.cpp
    module_count.cpp
    points.cpp
    preview.cpp
    shape.cpp
)

add_executable(rofi-tool ${ROFI_TOOL_SRCS} ${MODEL_RESOURCES})
target_link_libraries(rofi-tool PRIVATE dimcli atoms-heavy configuration parsing shapereconfig ${VTK_LIBRARIES})
