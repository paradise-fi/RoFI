cmake_minimum_required(VERSION 3.11)


set(RESOURCE_FILES
    model/body.obj
    model/connector.obj
    model/shoe.obj
)

set(FILES
    src/simplesim_client.cpp
)

add_resources(modelResources ${RESOURCE_FILES})

add_library(simplesimClient ${FILES} ${modelResources})
target_link_libraries(simplesimClient PUBLIC configuration simplesimConfigMsgs ${VTK_LIBRARIES})
target_include_directories(simplesimClient SYSTEM PUBLIC ${VTK_INCLUDE_DIRS})
target_include_directories(simplesimClient PUBLIC include)
