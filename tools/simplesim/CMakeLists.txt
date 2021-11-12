cmake_minimum_required(VERSION 3.10)


add_resources(modelResources
    "model/body.obj" "model/connector.obj" "model/shoe.obj")

set (SRC
    main.cpp
    simplesim_client.cpp
    ${modelResources}
)

add_executable(rofi-simplesim ${SRC})
target_link_libraries(rofi-simplesim simplesim gzMessageServer configuration ${VTK_LIBRARIES} dimcli)
