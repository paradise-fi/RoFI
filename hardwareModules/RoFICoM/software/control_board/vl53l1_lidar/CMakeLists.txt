cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")

set(ROFICOM_MCU "STM32G071xx")
set(ROFICOM_MCU_SPEC "STM32G071GBUx")

file(GLOB_RECURSE LIDAR ./*.cpp ./*.c)
message(STATUS "Files are ${LIDAR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")


# setup_stm32cxx("g0" ${ROFICOM_MCU} ${ROFICOM_MCU_SPEC})

add_stm32_target(
        LIB
        TARGET lidar
        LIBTYPE STATIC
        MCU ${ROFICOM_MCU}
        MCU_SPEC ${ROFICOM_MCU_SPEC}
        LINKER_SCRIPT $ENV{ROFI_ROOT}/hardwareModules/RoFICoM/software/control_board/stm32/STM32G071GBUx_FLASH.ld
        FILES ${LIDAR})
target_include_directories(lidar PUBLIC
        .
        ./core/
        ./platform/)
target_link_libraries(lidar
        g0_stm32cxx_i2c g0_stm32cxx_gpio atoms)
target_compile_definitions(lidar PUBLIC PAL_EXTENDED=1)

