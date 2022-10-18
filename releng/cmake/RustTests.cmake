function(add_cargo_package_tests TARGET_NAME)
    set(ONE_VALUE_KEYWORDS PACKAGE_NAME MANIFEST_PATH)
    cmake_parse_arguments(CART "${}" "${ONE_VALUE_KEYWORDS}" "${}" ${ARGN})

    if (NOT CART_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME is a required keyword for add_cargo_package_tests")
    endif()
    if (NOT CART_MANIFEST_PATH)
        message(FATAL_ERROR "MANIFEST_PATH is a required keyword for add_cargo_package_tests")
    endif()

    if (NOT IS_ABSOLUTE "${CART_MANIFEST_PATH}")
        set(CART_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${CART_MANIFEST_PATH})
    endif()
    get_filename_component(CART_MANIFEST_DIR ${CART_MANIFEST_PATH} DIRECTORY)

    # Based on location by Corrosion ("${CMAKE_BINARY_DIR}/cargo/build"),
    # but having it the same results in re-compiling every time (probably due to different flags)
    # TODO: Make the folders equal so dependencies are compiled only once
    set(CARGO_TARGET_DIR "${CMAKE_BINARY_DIR}/cargo/tests")

    if (CMAKE_BUILD_TYPE STREQUAL "" OR CMAKE_BUILD_TYPE STREQUAL Debug)
        set(CARGO_BUILD_TYPE debug)
    else()
        set(CARGO_BUILD_TYPE release)
    endif()

    add_custom_target(
        ${TARGET_NAME}
        ALL

        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        # Build test
        COMMAND
            ${CMAKE_COMMAND} -E env
            cargo
                test
                --no-run
                --manifest-path "${CART_MANIFEST_PATH}"
                --package "${CART_PACKAGE_NAME}"
                --target-dir "${CARGO_TARGET_DIR}"
                --message-format=json

            | _extract_rust_executable.py
            | xargs -i ${CMAKE_COMMAND} -E copy {} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}"
        WORKING_DIRECTORY "${CART_MANIFEST_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )

endfunction()
