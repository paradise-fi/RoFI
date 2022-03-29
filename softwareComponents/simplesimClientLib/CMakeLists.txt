cmake_minimum_required(VERSION 3.11)


set(RESOURCE_FILES
    model/body.obj
    model/connector.obj
    model/shoe.obj
)

set(FILES
    # Required to inlcude header files for Qt compilation
    include/simplesim_client.hpp
    include/changecolor.hpp

    src/simplesim_client.cpp
    src/changecolor.cpp

    src/mainwindow.ui
    src/changecolor.ui
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_resources(modelResources ${RESOURCE_FILES})

add_library(simplesimClient ${FILES} ${modelResources})
target_link_libraries(simplesimClient PUBLIC configuration simplesimConfigMsgs
        configurationWithJson ${VTK_LIBRARIES} Qt5::Core Qt5::Widgets)
target_include_directories(simplesimClient SYSTEM PUBLIC ${VTK_INCLUDE_DIRS})
target_include_directories(simplesimClient PUBLIC include)
