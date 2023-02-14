set(CARGO_BUILD_TYPE
    $<$<CONFIG:Debug>:dev>
    $<$<CONFIG:Release>:release>
    $<$<CONFIG:RelWithDebInfo>:rel-with-deb-info>
    $<$<CONFIG:MinSizeRel>:min-size-rel>
)

function(add_rust_library TARGET_NAME)
    set(FLAG_KEYWORDS ALL_FEATURES)
    set(ONE_VALUE_KEYWORDS MANIFEST_PATH)
    set(MULTI_VALUES_KEYWORDS FEATURES)
    cmake_parse_arguments(RRC "${FLAG_KEYWORDS}" "${ONE_VALUE_KEYWORDS}" "${MULTI_VALUES_KEYWORDS}" ${ARGN})

    if(RRC_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${RRC_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT DEFINED RRC_MANIFEST_PATH)
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
    elseif (NOT IS_ABSOLUTE "${RRC_MANIFEST_PATH}")
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${RRC_MANIFEST_PATH})
    endif()

    if(RRC_FEATURES)
        set(FEATURES_FLAG --features "${RRC_FEATURES}")
    else()
        set(FEATURES_FLAG "")
    endif()

    if(RRC_ALL_FEATURES)
        if(RRC_FEATURES)
            message(FATAL_ERROR "ALL_FEATURES and FEATURES are exclusive")
        endif()
        set(ALL_FEATURES_FLAG --all-features)
    else()
        set(ALL_FEATURES_FLAG "")
    endif()

    set(TARGET_LIBRARY_FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    add_custom_target(
        ${TARGET_NAME}-build
        ALL
        COMMAND
            cargo
                build
                --lib
                --package "${TARGET_NAME}"
                -Z unstable-options
                --out-dir "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"

                --profile ${CARGO_BUILD_TYPE}
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
        BYPRODUCTS
            ${TARGET_LIBRARY_FILE}
    )

    add_library(${TARGET_NAME} INTERFACE)
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}-build)
    target_link_libraries(${TARGET_NAME} INTERFACE "${TARGET_LIBRARY_FILE}")
endfunction()

function(add_rust_executable TARGET_NAME)
    set(FLAG_KEYWORDS ALL_FEATURES)
    set(ONE_VALUE_KEYWORDS MANIFEST_PATH)
    set(MULTI_VALUES_KEYWORDS FEATURES)
    cmake_parse_arguments(RRC "${FLAG_KEYWORDS}" "${ONE_VALUE_KEYWORDS}" "${MULTI_VALUES_KEYWORDS}" ${ARGN})

    if(NOT DEFINED RRC_MANIFEST_PATH)
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
    elseif (NOT IS_ABSOLUTE "${RRC_MANIFEST_PATH}")
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${RRC_MANIFEST_PATH})
    endif()

    if(RRC_FEATURES)
        set(FEATURES_FLAG --features "${RRC_FEATURES}")
    else()
        set(FEATURES_FLAG "")
    endif()

    if(RRC_ALL_FEATURES)
        if(RRC_FEATURES)
            message(FATAL_ERROR "ALL_FEATURES and FEATURES are exclusive")
        endif()
        set(ALL_FEATURES_FLAG --all-features)
    else()
        set(ALL_FEATURES_FLAG "")
    endif()

    add_custom_target(
        ${TARGET_NAME}
        ALL
        COMMAND
            cargo
                build
                --bin "${TARGET_NAME}"
                -Z unstable-options
                --out-dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"

                --profile ${CARGO_BUILD_TYPE}
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

function(add_rust_tests TARGET_NAME)
    set(FLAG_KEYWORDS ALL_FEATURES)
    set(ONE_VALUE_KEYWORDS MANIFEST_PATH PACKAGE)
    set(MULTI_VALUES_KEYWORDS FEATURES)
    cmake_parse_arguments(RRC "${FLAG_KEYWORDS}" "${ONE_VALUE_KEYWORDS}" "${MULTI_VALUES_KEYWORDS}" ${ARGN})

    if(NOT DEFINED RRC_MANIFEST_PATH)
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
    elseif (NOT IS_ABSOLUTE "${RRC_MANIFEST_PATH}")
        set(RRC_MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${RRC_MANIFEST_PATH})
    endif()

    if(RRC_FEATURES)
        set(FEATURES_FLAG --features "${RRC_FEATURES}")
    else()
        set(FEATURES_FLAG "")
    endif()

    if(RRC_ALL_FEATURES)
        if(RRC_FEATURES)
            message(FATAL_ERROR "ALL_FEATURES and FEATURES are exclusive")
        endif()
        set(ALL_FEATURES_FLAG --all-features)
    else()
        set(ALL_FEATURES_FLAG "")
    endif()

    if(DEFINED RRC_PACKAGE)
        set(PACKAGE_FLAG --package "${RRC_PACKAGE}")
    else()
        set(PACKAGE_FLAG --workspace)
    endif()

    set(COMPILER_ARTIFACTS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_artifacts.json")

    add_custom_target(
        ${TARGET_NAME}
        ALL
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
        COMMAND
            cargo
                test
                --no-run
                ${PACKAGE_FLAG}
                --message-format=json-render-diagnostics

                --profile ${CARGO_BUILD_TYPE}
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
            > "${COMPILER_ARTIFACTS_FILE}"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND
            cat "${COMPILER_ARTIFACTS_FILE}" | _get_rust_executables.py script "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()
