cmake_minimum_required(VERSION 3.10)

find_package(Doxygen REQUIRED)

set(extractDoxygen ${CMAKE_COMMAND} -E env DOXYGEN="$<TARGET_FILE:Doxygen::doxygen>" "$ENV{ROFI_ROOT}/releng/doc/extractDoxygen.sh")

function(add_doxygen_source lib_name source_path)
    file(GLOB_RECURSE src
        "${source_path}/*.cpp" "${source_path}/*.c"
        "${source_path}/*.hpp" "${source_path}/*.h")
    set(dest_path "${CMAKE_CURRENT_BINARY_DIR}/doxygen/${lib_name}")
    set(index_file ${dest_path}/xml/index.xml)
    file(MAKE_DIRECTORY "${dest_path}")
    add_custom_command(OUTPUT "${index_file}" PRE_BUILD
        COMMAND ${extractDoxygen} ${lib_name} ${source_path} ${dest_path}
        DEPENDS ${src}
        COMMENT "Extracting Doxygen for ${lib_name}")
endfunction()

function(add_sphinx_target target_name)
    cmake_parse_arguments(PARSE_ARGV 1
        ADD_SPINX_TARGET_P
        ""
        "SOURCE;DESTINATION"
        "DOXYGEN"
    )

    set(doxygen_deps ${ADD_SPINX_TARGET_P_DOXYGEN})
    foreach(dox ${doxygen_deps})
        list(APPEND defs "-Dbreathe_projects.${dox}=${CMAKE_CURRENT_BINARY_DIR}/doxygen/${dox}/xml")
    endforeach()

    file(GLOB_RECURSE src "${ADD_SPINX_TARGET_P_SOURCE}/*")
    foreach(dox ${doxygen_deps})
        list(APPEND doxygen_src "${CMAKE_CURRENT_BINARY_DIR}/doxygen/${dox}/xml/index.xml")
    endforeach()

    set(web_index "${ADD_SPINX_TARGET_P_DESTINATION}/index.html")

    add_custom_command(OUTPUT ${web_index} PRE_BUILD
        COMMAND sphinx-build -b html ${defs}
            "${ADD_SPINX_TARGET_P_SOURCE}" "${ADD_SPINX_TARGET_P_DESTINATION}"
        DEPENDS ${src} ${doxygen_src}
        COMMENT "Building Sphinx page for ${target_name}")
    list(TRANSFORM doxygen_deps PREPEND "doxygen_")

    add_custom_target(${target_name} ALL
        DEPENDS "${web_index}")
endfunction()