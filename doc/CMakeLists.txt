cmake_minimum_required(VERSION 3.12)

include(Spinx)

add_doxygen_source(driver
    $ENV{ROFI_ROOT}/softwareComponents/rofiHalPhys)
add_doxygen_source(roficom
    $ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board)
add_doxygen_source(lib
    $ENV{ROFI_ROOT}/softwareComponents)
add_doxygen_source(sim
    $ENV{ROFI_ROOT}/RoFISim)

add_sphinx_target(doc
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}"
    DESTINATION "${DOC_PATH}"
    DOXYGEN driver roficom lib sim)
