cmake_minimum_required(VERSION 3.6)
project(rofi)

add_resources(modelResources
    "model/body.obj" "model/connector.obj" "model/shoe.obj" "model/point.obj" )

add_executable(rofi-tool main.cpp points.cpp preview.cpp build.cpp rendering.cpp check.cpp
    ${modelResources})
target_link_libraries(rofi-tool PRIVATE dimcli configuration isoreconfig ${VTK_LIBRARIES} atoms-heavy)
