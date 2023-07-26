include($ENV{IDF_PATH}/tools/cmake/idf.cmake)
include($ENV{IDF_PATH}/tools/cmake/toolchain-esp32.cmake)

set(IMG_DIR ${CMAKE_BINARY_DIR}/img)
file(MAKE_DIRECTORY "${IMG_DIR}")

function(generate_image_from A_ELF_TARGET A_FLASH_SIZE)
    get_filename_component(NAME_BASE "${A_ELF_TARGET}" NAME_WLE)
    set(IMG_NAME "${NAME_BASE}.app")
    set(ELF_NAME "${NAME_BASE}.elf")
    set(TSTAMP "${A_ELF_TARGET}.bin_timestamp")

    # Set correct flash size
    set(FLASH_ARGS ${esptool_elf2image_args})
    list(FIND FLASH_ARGS "--flash_size" F_IDX)
    if ( NOT ${F_IDX} EQUAL -1 )
        MATH(EXPR F_IDX_PP "${F_IDX} + 1")
        list(REMOVE_AT FLASH_ARGS ${F_IDX_PP})
        list(INSERT FLASH_ARGS ${F_IDX_PP} "${A_FLASH_SIZE}")
    endif()

    add_custom_command(OUTPUT "${TSTAMP}"
        COMMAND ${ESPTOOLPY} elf2image ${FLASH_ARGS}
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

function(set_partition_table target tablefile)
    get_filename_component(NAME_BASE "${target}" NAME_WLE)
    set(TABLE_BIN_NAME "${NAME_BASE}.table")
    add_custom_command(OUTPUT "${IMG_DIR}/${TABLE_BIN_NAME}"
        COMMAND python $ENV{IDF_PATH}/components/partition_table/gen_esp32part.py
                        "${tablefile}" "${IMG_DIR}/${TABLE_BIN_NAME}"
        DEPENDS "${tablefile}"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating partition table image ${TABLE_BIN_NAME}")
    add_custom_target("${TABLE_BIN_NAME}" DEPENDS "${IMG_DIR}/${TABLE_BIN_NAME}")
    add_dependencies("${target}" "${TABLE_BIN_NAME}")
endfunction()

function(fatfs_create_spiflash_image_dir TARGET PARTITION_NAME BASE_DIR PARTITION_SIZE)
    message(STATUS "fatfs_create_spiflash_image_dir: ${TARGET} ${PARTITION_NAME} ${BASE_DIR}")
    get_filename_component(NAME_BASE "${TARGET}" NAME_WLE) # remove extension (umProject.elf -> umProject)
    set(IMG_NAME "${NAME_BASE}.${PARTITION_NAME}") # umProject.{PARTITION_NAME} (umProject.fatfs)
    set(TSTAMP "${IMG_NAME}.bin_timestamp") # save timestamp to umProject.fatfs.bin_timestamp for incremental build


    # // print variable_requires
    # message(STATUS "TARGET: ${TARGET}" "${PARTITION_NAME}" "${BASE_DIR}" "${PARTITION_SIZE}")
    # message(STATUS "PWD: ${CMAKE_CURRENT_SOURCE_DIR}")
    # message(STATUS "IMG_NAME: ${IMG_NAME}" "TSTAMP: ${TSTAMP}")

    add_custom_command(OUTPUT "${TSTAMP}" # run this
        COMMAND python $ENV{IDF_PATH}/components/fatfs/fatfsgen.py
         "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_DIR}" --output "${IMG_DIR}/${IMG_NAME}" --long_name_support --partition_size "${PARTITION_SIZE}"
        COMMAND ${CMAKE_COMMAND} -E echo "${IMG_DIR}/${IMG_NAME}" # print path to file
        COMMAND ${CMAKE_COMMAND} -E md5sum "${IMG_DIR}/${IMG_NAME}" > "${TSTAMP}"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating FAT partition image ${IMG_NAME}"
        )
    add_custom_target("${PARTITION_NAME}" DEPENDS "${TSTAMP}")
    set_target_properties("${PARTITION_NAME}" PROPERTIES EXCLUDE_FROM_ALL "exclude-NOTFOUND")
endfunction()

function(spiffs_create_spiflash_image TARGET PARTITION_NAME BASE_DIR PARTITION_SIZE)
    message(STATUS "spiffs_create_spiflash_image: ${TARGET} ${PARTITION_NAME} ${BASE_DIR}")
    get_filename_component(NAME_BASE "${TARGET}" NAME_WLE) # remove extension (umProject.elf -> umProject)
    set(IMG_NAME "${NAME_BASE}.${PARTITION_NAME}") # umProject.{PARTITION_NAME} (umProject.spiffs)
    set(TSTAMP "${IMG_NAME}.bin_timestamp") # save timestamp to umProject.spiffs.bin_timestamp for incremental build


    # // print variable_requires
    # message(STATUS "TARGET: ${TARGET}")
    # message(STATUS "PARTITION_NAME: ${PARTITION_NAME}")
    # message(STATUS "BASE_DIR: ${BASE_DIR}")
    # message(STATUS "PWD: ${CMAKE_CURRENT_SOURCE_DIR}")
    # message(STATUS "IMG_OUTPUT: ${IMG_DIR}/${IMG_NAME}")

    add_custom_command(OUTPUT "${TSTAMP}" # run this
        COMMAND python $ENV{IDF_PATH}/components/spiffs/spiffsgen.py
        "${PARTITION_SIZE}" "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_DIR}" "${IMG_DIR}/${IMG_NAME}"
        COMMAND ${CMAKE_COMMAND} -E echo "${IMG_DIR}/${IMG_NAME}" # print path to file
        COMMAND ${CMAKE_COMMAND} -E md5sum "${IMG_DIR}/${IMG_NAME}" > "${TSTAMP}"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating SPIFFS partition image ${IMG_NAME}"
        )
    add_custom_target("${PARTITION_NAME}" DEPENDS "${TSTAMP}")
    set_target_properties("${PARTITION_NAME}" PROPERTIES EXCLUDE_FROM_ALL "exclude-NOTFOUND")
endfunction()