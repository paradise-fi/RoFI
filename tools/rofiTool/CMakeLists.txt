cmake_minimum_required(VERSION 3.6)
project(rofi)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

add_resources(modelResources
    "model/body.obj" "model/connector.obj" "model/shoe.obj")

add_executable(rofi-tool main.cpp preview.cpp
    ${modelResources})
target_link_libraries(rofi-tool PRIVATE dimcli configuration ${VTK_LIBRARIES} atoms-heavy)

