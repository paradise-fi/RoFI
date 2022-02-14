include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
include($ENV{IDF_PATH}/tools/cmake/toolchain-esp32.cmake)

set(IMG_DIR ${CMAKE_BINARY_DIR}/img)
file(MAKE_DIRECTORY "${IMG_DIR}")

function(generate_image_from A_ELF_TARGET)
    get_filename_component(NAME_BASE "${A_ELF_TARGET}" NAME_WLE)
    set(IMG_NAME "${NAME_BASE}.bin")
    set(ELF_NAME "${NAME_BASE}.elf")
    set(TSTAMP "${A_ELF_TARGET}.bin_timestamp")

    add_custom_command(OUTPUT "${TSTAMP}"
        COMMAND ${ESPTOOLPY} elf2image ${ESPTOOLPY_FLASH_OPTIONS} ${esptool_elf2image_args}
            -o "${IMG_DIR}/${IMG_NAME}" "$<TARGET_FILE:${A_ELF_TARGET}>"
        COMMAND ln -s -f "$<TARGET_FILE:${A_ELF_TARGET}>" "${IMG_DIR}/${ELF_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "${IMG_DIR}/${IMG_NAME}"
        COMMAND ${CMAKE_COMMAND} -E md5sum "${IMG_DIR}/${IMG_NAME}" > "${TSTAMP}"
        DEPENDS ${A_ELF_TARGET}
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating binary image ${IMG_NAME}"
        )
    add_custom_target("${NAME_BASE}" DEPENDS "${TSTAMP}" "bootloader" "partition_table_bin")
    set_target_properties("${NAME_BASE}" PROPERTIES EXCLUDE_FROM_ALL "exclude-NOTFOUND")
endfunction()
