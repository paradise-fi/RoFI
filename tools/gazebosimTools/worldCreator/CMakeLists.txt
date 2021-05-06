cmake_minimum_required(VERSION 3.11)


# If you change the cached variables, run a clean compilation

set(ROFI_EMPTY_WORLD_FILE
        $ENV{ROFI_ROOT}/data/gazebo/worlds/empty.world
    CACHE
        FILEPATH
        "The empty world for world creator."
    )

set(ROFI_SDF_MODEL_FILES
        $ENV{ROFI_ROOT}/data/gazebo/models/roficom/model.sdf
        $ENV{ROFI_ROOT}/data/gazebo/models/universalModule/model.sdf
    CACHE
        FILEPATH
        "The paths to the sdf models used by the world creator."
    )


add_executable(rofi-gazebosim-worldCreator worldCreatorMain.cpp worldCreator.cpp)
target_link_libraries(rofi-gazebosim-worldCreator legacy-configuration rofiPluginCommon ${GAZEBO_LIBRARIES} ${Boost_LIBRARIES})
target_include_directories(rofi-gazebosim-worldCreator SYSTEM PRIVATE ${GAZEBO_INCLUDE_DIRS})
target_compile_definitions(rofi-gazebosim-worldCreator PUBLIC WORLD_FILE=${ROFI_EMPTY_WORLD_FILE})


function(add_gazebo_world world_file input_file)
    add_custom_command(
        OUTPUT
            ${CMAKE_BINARY_DIR}/data/gazebo/worlds/${world_file}
        COMMAND
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-gazebosim-worldCreator
                -i ${CMAKE_CURRENT_SOURCE_DIR}/${input_file}
                -o ${CMAKE_BINARY_DIR}/data/gazebo/worlds/${world_file}
        DEPENDS
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/rofi-gazebosim-worldCreator
            ${CMAKE_CURRENT_SOURCE_DIR}/${input_file}
            ${ROFI_EMPTY_WORLD_FILE}
            ${ROFI_SDF_MODEL_FILES}
    )

    # We cannot name the target the same as the file; Ninja will complain
    add_custom_target(
            gz-world-${world_file}
        ALL
        DEPENDS
            ${CMAKE_BINARY_DIR}/data/gazebo/worlds/${world_file}
    )
    add_dependencies(createGazeboWorlds gz-world-${world_file})
endfunction()

add_custom_target(createGazeboWorlds ALL)
