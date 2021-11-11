cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")

project(control_board C CXX ASM)

set(ROFICOM_MCU "STM32G071xx")
set(ROFICOM_MCU_SPEC "STM32G071GBUx")

file(GLOB_RECURSE SRC src/*.cpp src/*.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")

setup_stm32cxx("g0" ${ROFICOM_MCU} ${ROFICOM_MCU_SPEC})

add_stm32_target(
        EXECUTABLE
        TARGET roficomFw
        MCU ${ROFICOM_MCU}
        MCU_SPEC ${ROFICOM_MCU_SPEC}
        LINKER_SCRIPT $ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board/stm32/STM32G071GBUx_FLASH.ld
        FILES ${SRC})
set_property(TARGET roficomFw PROPERTY CXX_STANDARD 17)
target_include_directories(roficomFw PRIVATE src)
target_compile_options(roficomFw PRIVATE
        -Wall -Wextra -pedantic -funwind-tables)
target_link_libraries(roficomFw
        g0_startup function2 g0_stm32cxx g0_HAL_CRC
        g0_stm32cxx_system g0_stm32cxx_uart g0_stm32cxx_spi g0_stm32cxx_gpio
        g0_stm32cxx_dma g0_stm32cxx_adc g0_stm32cxx_timer)

# add_executable(${PROJECT_NAME}.elf ${SRC})
# set_property(TARGET ${PROJECT_NAME}.elf PROPERTY CXX_STANDARD 17)
# target_include_directories(${PROJECT_NAME}.elf PRIVATE src)
# target_compile_options(${PROJECT_NAME}.elf PRIVATE -Wall -Wextra -pedantic -funwind-tables)
# target_link_options(${PROJECT_NAME}.elf PRIVATE "-Wl,-Map=${PROJECT_NAME}.map,--cref" "-Wl,--print-memory-usage" "-funwind-tables" "-fasynchronous-unwind-tables")
# target_link_libraries(${PROJECT_NAME}.elf stm32cxx function2)
# # set_target_properties(${PROJECT_NAME}.elf PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

# set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
# set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
# message(${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
# add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
#         COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
#         COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

# add_custom_target(flash
#     COMMAND $ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board/tools/flashAll.sh --format ihex write ${HEX_FILE}
#     DEPENDS ${HEX_FILE})
