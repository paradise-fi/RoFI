cmake_minimum_required(VERSION 3.6)

project(control_board C CXX ASM)

set(BIOS_MCU "STM32F411xE")

file(GLOB_RECURSE SRC src/*.cpp src/*.c config/*.c config/*.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")

setup_stm32cube("f4" ${BIOS_MCU})
setup_chibiHal("f4" ${BIOS_MCU})

add_stm32_target(
        EXECUTABLE
        TARGET umBios2
        MCU ${BIOS_MCU}
        # LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F411CCUx_FLASH.ld
        FILES ${SRC})
set_property(TARGET umBios2 PROPERTY CXX_STANDARD 17)
target_link_libraries(umBios2 PRIVATE
        f4_ChibiStartup f4_freertos f4_ChibiFull)
target_include_directories(umBios2 PUBLIC src config)