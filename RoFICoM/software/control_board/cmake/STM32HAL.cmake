add_definitions(-DUSE_HAL_DRIVER)


string(TOLOWER ${MCU} MCU_LOWER)
string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)

file(GLOB_RECURSE HAL_SOURCES ${STM_LIB}/${MCU_FAMILY}_HAL_Driver/Src/*.c)
file(GLOB_RECURSE CMSIS_SYSTEM ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
file(GLOB_RECURSE CMSIS_STARTUP ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/startup_${MCU_LOWER}.s)

include_directories(SYSTEM ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)
include_directories(SYSTEM ${STM_LIB}/CMSIS/Include)
include_directories(SYSTEM ${STM_LIB}/${MCU_FAMILY}_HAL_Driver/Inc)

add_library(stm32_hal STATIC ${HAL_SOURCES} ${CMSIS_SYSTEM} ${CMSIS_STARTUP})

add_definitions(-DUSE_FULL_LL_DRIVER)