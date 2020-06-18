cmake_minimum_required(VERSION 3.11)


# If you change the cached variables, run a clean compilation

set(EMPTY_WORLD_FILE
        ${CMAKE_SOURCE_DIR}/worlds/empty.world
    CACHE
        FILEPATH
        "The empty world for world creator."
    )

set(SDF_MODEL_FILES
        ${CMAKE_SOURCE_DIR}/models/universalModule/model.sdf
        ${CMAKE_SOURCE_DIR}/models/roficom/model.sdf
    CACHE
        FILEPATH
        "The paths to the sdf models used by the world creator."
    )


add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../RoFILib/configuration ${CMAKE_CURRENT_BINARY_DIR}/configuration)


add_library(worldCreatorLib STATIC worldCreator.cpp)
target_link_libraries(worldCreatorLib configuration rofiPluginCommon ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES} armadillo)

add_executable(worldCreator worldCreatorMain.cpp)
target_link_libraries(worldCreator worldCreatorLib)
target_compile_definitions(worldCreator PUBLIC WORLD_FILE=${EMPTY_WORLD_FILE})


function(create_world world_file input_file)
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/${world_file}
        COMMAND
            ${CMAKE_SOURCE_DIR}/setGazeboVariables.sh
        ARGS
            ${CMAKE_BINARY_DIR}/tools/worldCreator/worldCreator
            -i ${CMAKE_CURRENT_SOURCE_DIR}/${input_file}
            -o ${CMAKE_CURRENT_BINARY_DIR}/${world_file}
        DEPENDS
            worldCreator
            ${CMAKE_CURRENT_SOURCE_DIR}/${input_file}
            ${EMPTY_WORLD_FILE}
            ${SDF_MODEL_FILES}
    )

    add_custom_target(
            ${world_file}
        ALL
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/${world_file}
    )
endfunction()
