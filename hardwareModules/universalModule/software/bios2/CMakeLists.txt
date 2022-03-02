cmake_minimum_required(VERSION 3.12)

project(control_board C CXX ASM)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O2")

set(BIOS_MCU "STM32F411xE")

file(GLOB_RECURSE SRC src/*.cpp src/*.c config/*.c config/*.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5 -fconcepts")

setup_stm32cube("f4" ${BIOS_MCU})
setup_stm32cxx("f4" ${BIOS_MCU})

add_stm32_target(
        EXECUTABLE
        TARGET umBios2Fw
        MCU ${BIOS_MCU}
        LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F411CEUx_FLASH.ld
        FILES ${SRC})
target_compile_options(umBios2Fw PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(umBios2Fw PUBLIC "STM32F411CEUx")
target_link_options(umBios2Fw PUBLIC "-Wl,-Map=umBios2Fw.map")
set_property(TARGET umBios2Fw PROPERTY CXX_STANDARD 20)
target_link_libraries(umBios2Fw PRIVATE
        f4_startup
        f4_LL f4_HAL f4_HAL_Gpio
        f4_stm32cxx f4_stm32cxx_adc f4_stm32cxx_uart f4_stm32cxx_gpio
        f4_stm32cxx_system f4_stm32cxx_usb f4_stm32cxx_timer)
target_compile_definitions(umBios2Fw PUBLIC "-DUSBD_SOF_DISABLED" "-DHSE_VALUE=24000000")
target_include_directories(umBios2Fw PUBLIC src config)