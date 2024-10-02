cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

# The flags `-fstack-protector-all` and `-fstack-check` are added due to bugs discovered in development
# where the stack got corrupted, this was fixed by reducing packet buckets.
# The flag `-Os` in debug build is to assure the debug build can be fitted to the mcu FLASH memory.
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fstack-protector-all -fstack-check -Os")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fstack-protector-all -fstack-check -Os")

project(control_board C CXX ASM)

set(ROFICOM_MCU "STM32G071xx")
set(ROFICOM_MCU_SPEC "STM32G071GBUx")

add_subdirectory(vl53l1_lidar)

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
set_property(TARGET roficomFw PROPERTY CXX_STANDARD 20)
target_include_directories(roficomFw PRIVATE src)
target_compile_options(roficomFw PRIVATE
        -Wall -Wextra -pedantic -funwind-tables)
target_link_libraries(roficomFw
        g0_startup function2 g0_stm32cxx g0_HAL_CRC
        g0_stm32cxx_system g0_stm32cxx_uart g0_stm32cxx_spi g0_stm32cxx_gpio
        g0_stm32cxx_dma g0_stm32cxx_adc g0_stm32cxx_timer g0_stm32cxx_i2c
        lidar atoms)
target_compile_definitions(roficomFw PUBLIC GPIOD_BASE)
