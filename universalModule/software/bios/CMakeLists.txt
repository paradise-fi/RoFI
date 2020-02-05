cmake_minimum_required(VERSION 3.9)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../releng/cmake")

set(MCU "STM32F030x8")
set(STM_LIB ${CMAKE_SOURCE_DIR}/stm32)
set(MCU_LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/stm32/STM32F030C8Tx_FLASH.ld)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(CMAKE_TOOLCHAIN_FILE STM32Toolchain)
include(STM32Device)
include(ClangTidy)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")

# Set pool memory size
set(MEMORY_BLOCK_SIZE 128)
set(MEMORY_BLOCK_COUNT 2)
add_subdirectory(stm32cxx)

project(umBios C CXX ASM)

file(GLOB_RECURSE SRC src/*.cpp src/*.c)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=5")

add_executable(${PROJECT_NAME}.elf ${SRC})
set_property(TARGET ${PROJECT_NAME}.elf PROPERTY CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME}.elf PRIVATE src)
target_compile_options(${PROJECT_NAME}.elf PRIVATE -Wall -Wextra -pedantic -funwind-tables)
target_link_options(${PROJECT_NAME}.elf PRIVATE "-Wl,-Map=${PROJECT_NAME}.map,--cref" "-Wl,--print-memory-usage" "-funwind-tables" "-fasynchronous-unwind-tables")
target_link_libraries(${PROJECT_NAME}.elf PRIVATE "-Wl,--whole-archive" stm32cxx "-Wl,--no-whole-archive" function2)
set_target_properties(${PROJECT_NAME}.elf PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")

set(HEX_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.bin)
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")

add_custom_target(flash
    COMMAND st-flash --format ihex write ${HEX_FILE}
    DEPENDS ${HEX_FILE})
