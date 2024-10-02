function(add_executable_script SCRIPT_PATH)
    set(FLAG_KEYWORDS BASH)
    set(MULTI_VALUE_KEYWORDS DEPENDS)
    cmake_parse_arguments(SCR "${FLAG_KEYWORDS}" "" "${MULTI_VALUE_KEYWORDS}" ${ARGN})
    get_filename_component(filename "${SCRIPT_PATH}" NAME)
    get_filename_component(abspath "${SCRIPT_PATH}" ABSOLUTE)

    add_custom_command(
        OUTPUT
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${filename}"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${abspath}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND
            chmod +x "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${filename}"
        DEPENDS
            "${abspath}" ${SCR_DEPENDS}
    )
    add_custom_target("${filename}" ALL DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${filename}")
    if(SCR_BASH)
        add_custom_command(
            TARGET "${filename}"
            PRE_BUILD
            COMMAND bash -n "${abspath}"
        )
    endif()
endfunction()
