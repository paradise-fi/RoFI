get_filename_component(
    RMAKE_RUST_GET_EXECUTABLES_TOOL
    "${CMAKE_CURRENT_LIST_DIR}/../tools/_get_rust_executables.py"
    ABSOLUTE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_PROFILE_NAME dev)
    set(CARGO_PROFILE_DIR debug)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CARGO_PROFILE_NAME release)
    set(CARGO_PROFILE_DIR release)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CARGO_PROFILE_NAME rel-with-deb-info)
    set(CARGO_PROFILE_DIR rel-with-deb-info)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CARGO_PROFILE_NAME min-size-rel)
    set(CARGO_PROFILE_DIR min-size-rel)
else()
    message(FATAL_ERROR "Unsupported CMAKE_BUILD_TYPE for Rust: ${CMAKE_BUILD_TYPE}")
endif()

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

    set(BUILT_LIBRARY_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CARGO_PROFILE_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set(TARGET_LIBRARY_FILE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")

    add_custom_target(
        ${TARGET_NAME}-build
        ALL
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
        COMMAND
            cargo
                build
                --lib
                --package "${TARGET_NAME}"
                --profile "${CARGO_PROFILE_NAME}"
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${BUILT_LIBRARY_FILE}" "${TARGET_LIBRARY_FILE}"
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

    set(BUILT_EXECUTABLE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CARGO_PROFILE_DIR}/${TARGET_NAME}")
    set(TARGET_EXECUTABLE_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}")

    add_custom_target(
        ${TARGET_NAME}
        ALL
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND
            cargo
                build
                --bin "${TARGET_NAME}"
                --profile "${CARGO_PROFILE_NAME}"
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different "${BUILT_EXECUTABLE_FILE}" "${TARGET_EXECUTABLE_FILE}"
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

                --profile "${CARGO_PROFILE_NAME}"
                --manifest-path "${RRC_MANIFEST_PATH}"
                --target-dir "${CMAKE_CURRENT_BINARY_DIR}"
                ${ALL_FEATURES_FLAG}
                ${FEATURES_FLAG}
            > "${COMPILER_ARTIFACTS_FILE}"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND
            cat "${COMPILER_ARTIFACTS_FILE}" | python3 "${RMAKE_RUST_GET_EXECUTABLES_TOOL}" script "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        USES_TERMINAL
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()
