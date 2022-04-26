cmake_minimum_required(VERSION 3.12)

include(Spinx)

add_doxygen_source(roficom
    $ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board)
add_doxygen_source(lib
    $ENV{ROFI_ROOT}/softwareComponents)
add_doxygen_source(hal
    $ENV{ROFI_ROOT}/softwareComponents/rofiHalInc/include)
add_doxygen_source(configuration
    $ENV{ROFI_ROOT}/softwareComponents/configuration)

add_sphinx_target(doc
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}"
    DESTINATION "${DOC_PATH}"
    DOXYGEN roficom lib hal configuration)
