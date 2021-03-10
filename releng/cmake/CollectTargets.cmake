# Collect all project targets ignoring fetched dependencies and interface
# libraries
function(collect_targets var)
    set(targets)
    collect_targets_recursive(targets ${CMAKE_CURRENT_SOURCE_DIR})
    set(${var} ${targets} PARENT_SCOPE)
endfunction()

macro(collect_targets_recursive targets dir)
    if(NOT ("${dir}" MATCHES "${PROJECT_BINARY_DIR}/_deps/.*"))
        get_property(subdirectories DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
        foreach(subdir ${subdirectories})
            collect_targets_recursive(${targets} ${subdir})
        endforeach()

        get_property(current_targets DIRECTORY ${dir} PROPERTY BUILDSYSTEM_TARGETS)
        string(REGEX REPLACE "^$ENV{ROFI_ROOT}/" "" path ${dir})

        foreach(target ${current_targets})
            get_target_property(type ${target} TYPE)
            if (NOT ${type} STREQUAL "INTERFACE_LIBRARY")
                list(TRANSFORM target PREPEND "${path}:")
                list(APPEND ${targets} ${target})
            endif()
        endforeach()
    endif()
endmacro()

function(list_targets)
    collect_targets(targets)
    string(JOIN "\n" t ${targets})
    file(WRITE "${PROJECT_BINARY_DIR}/targets.txt" "${t}")
endfunction()