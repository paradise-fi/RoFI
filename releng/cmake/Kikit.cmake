find_program (KIKIT NAMES "kikit")

function(set_semicolon_safe varname)
    set(tmp "")
    foreach(X ${ARGN})
        string(REPLACE ";" "$<SEMICOLON>" Y "${X}")
        list(APPEND tmp "${Y}")
    endforeach()
    set(${varname} ${tmp} PARENT_SCOPE)
endfunction()

function(add_pcb ADD_PCB_TARGET)
    set(options STEPMODEL)
    set(oneValueArgs SOURCE SCHEMATICS)
    set(multiValueArgs PANELIZE SEPARATE FAB)

    foreach(X ${ARGN})
        string(REPLACE ";" "$<SEMICOLON>" Y "${X}")
        list(APPEND PP_ARGS "${Y}")
    endforeach()

    cmake_parse_arguments(ADD_PCB "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${PP_ARGS})

    if(${ADD_PCB_SOURCE} STREQUAL "")
        message(FATAL_ERROR "You have to specify SOURCE for add_pcb")
    endif()

    set(targetBoard "${CMAKE_CURRENT_BINARY_DIR}/${ADD_PCB_TARGET}.kicad_pcb")
    if(NOT "${ADD_PCB_SEPARATE}" STREQUAL "")
        add_custom_command(
            OUTPUT ${targetBoard}
            DEPENDS ${ADD_PCB_SOURCE}
            VERBATIM
            COMMENT "Separating board ${ADD_PCB_TARGET} from ${ADD_PCB_SOURCE}"
            COMMAND ${KIKIT} separate ${ADD_PCB_SEPARATE} ${ADD_PCB_SOURCE} ${targetBoard}
        )
    elseif(NOT "${ADD_PCB_PANELIZE}" STREQUAL "")
        add_custom_command(
            OUTPUT ${targetBoard}
            DEPENDS ${ADD_PCB_SOURCE}
            VERBATIM
            COMMENT "Panelizing board ${ADD_PCB_TARGET} from ${ADD_PCB_SOURCE}"
            COMMAND ${KIKIT} panelize ${ADD_PCB_PANELIZE} ${ADD_PCB_SOURCE} ${targetBoard}
        )
    else()
        set(targetBoard ${ADD_PCB_SOURCE})
    endif()

    if(NOT "${ADD_PCB_FAB}" STREQUAL "")
        set(targetDir "${CMAKE_CURRENT_BINARY_DIR}/${ADD_PCB_TARGET}_man")
        set(targetArchive "${targetDir}/${ADD_PCB_TARGET}_gerbers.zip")

        if (NOT "${ADD_PCB_SCHEMATICS}" STREQUAL "")
            set(schematicsFlag --schematic ${ADD_PCB_SCHEMATICS})
        endif()

        add_custom_command(
            OUTPUT ${targetArchive}
            BYPRODUCTS
                "${targetDir}/${ADD_PCB_TARGET}_pos.csv"
                "${targetDir}/${ADD_PCB_TARGET}_bom.csv"
            DEPENDS ${targetBoard} ${ADD_PCB_SCHEMATICS}
            COMMENT "Exporting manufacturing data for ${ADD_PCB_TARGET}"
            COMMAND ${KIKIT} fab ${ADD_PCB_FAB}
                ${schematicsFlag} --nametemplate "${ADD_PCB_TARGET}_{}"
                ${targetBoard} ${targetDir}
        )
    endif()

    if("${ADD_PCB_STEPMODEL}")
        set(stepModel "${CMAKE_CURRENT_BINARY_DIR}/${ADD_PCB_TARGET}.step")
        add_custom_command(
            OUTPUT ${stepModel}
            DEPENDS ${targetBoard}
            COMMENT "Exporting 3D model of ${ADD_PCB_TARGET}"
            COMMAND pcb2step -f -o ${stepModel} ${targetBoard}
        )
    endif()

    add_custom_target(${ADD_PCB_TARGET}
        ALL
        COMMENT "Board ${ADD_PCB_TARGET}"
        DEPENDS ${targetBoard} ${targetArchive} ${stepModel})
endfunction()
