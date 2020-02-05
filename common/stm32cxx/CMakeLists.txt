cmake_minimum_required(VERSION 3.13)

project(stm32cxx)
enable_language(C CXX ASM)

# STM32 HAL and CMSIS
string(TOLOWER ${MCU} MCU_LOWER)
string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)
set(STM_LIB ${CMAKE_CURRENT_SOURCE_DIR}/stm32)

file(GLOB_RECURSE HAL_SOURCES ${STM_LIB}/${MCU_FAMILY}_HAL_Driver/Src/*.c)
file(GLOB_RECURSE CMSIS_SYSTEM ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
file(GLOB_RECURSE CMSIS_STARTUP ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/startup_${MCU_LOWER}.s)

# add_library(stm32hal OBJECT ${HAL_SOURCES} ${CMSIS_SYSTEM} ${CMSIS_STARTUP})
# target_compile_options(stm32hal PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
# target_include_directories(stm32hal SYSTEM PUBLIC ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)
# target_include_directories(stm32hal SYSTEM PUBLIC ${STM_LIB}/CMSIS/Include)
# target_include_directories(stm32hal SYSTEM PUBLIC ${STM_LIB}/${MCU_FAMILY}_HAL_Driver/Inc)

# Function 2
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/extlib/function2")

# Printf
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/extlib/printf")

# STM32CXX
file(GLOB_RECURSE SRC src/*.cpp src/*.c)

add_library(stm32cxx STATIC ${SRC} ${HAL_SOURCES} ${CMSIS_SYSTEM} ${CMSIS_STARTUP})
target_compile_options(stm32cxx PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
set_property(TARGET stm32cxx PROPERTY CXX_STANDARD 17)
target_link_libraries(stm32cxx PUBLIC function2 PUBLIC printf)
target_include_directories(stm32cxx PUBLIC src)
target_include_directories(stm32cxx SYSTEM PUBLIC ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)
target_include_directories(stm32cxx SYSTEM PUBLIC ${STM_LIB}/CMSIS/Include)
target_include_directories(stm32cxx SYSTEM PUBLIC ${STM_LIB}/${MCU_FAMILY}_HAL_Driver/Inc)

# There is a pooled memory functionality in the library, if no values are set,
# define one
if(NOT DEFINED MEMORY_BLOCK_SIZE)
    set(MEMORY_BLOCK_SIZE 1024)
endif()

if(NOT DEFINED MEMORY_BLOCK_COUNT)
    set(MEMORY_BLOCK_COUNT 10)
endif()

target_compile_options(stm32cxx PUBLIC -DMEMORY_BLOCK_SIZE=${MEMORY_BLOCK_SIZE} -DMEMORY_BLOCK_COUNT=${MEMORY_BLOCK_COUNT})