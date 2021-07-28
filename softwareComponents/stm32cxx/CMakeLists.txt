cmake_minimum_required(VERSION 3.11)

include(FetchContent)

function(setup_stm32cxx PREFIX MCU)

    setup_mcu_details(${MCU})

    enable_language(C CXX ASM)

    # STM32 HAL and CMSIS
    string(TOLOWER ${MCU} MCU_LOWER)
    string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)

    # Fetch HAL libraries
    if(${MCU_FAMILY} STREQUAL "STM32F0xx")
        FetchContent_Declare(
            hal
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF0.git
            GIT_TAG        v1.11.0
        )
    elseif(${MCU_FAMILY} STREQUAL "STM32G0xx")
        FetchContent_Declare(
            hal
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeG0.git
            GIT_TAG        v1.3.0
        )
    else()
        message(FATAL_ERROR "Unsupported MCU family '${MCU_FAMILY}' specified." )
    endif()

    FetchContent_GetProperties("hal")
    if(NOT "${hal_POPULATED}")
    FetchContent_Populate("hal")
    # Turn config template into config
    file(GLOB_RECURSE cfgFile "${hal_SOURCE_DIR}/Drivers/*_conf_template.h")
    foreach(f ${cfgFile})
            string(REGEX REPLACE "_template\.h$" ".h" newF ${f})
            file(RENAME ${f} ${newF})
        endforeach(f)
    endif()

    set(STM_LIB ${hal_SOURCE_DIR}/Drivers)
    set(HAL_PATH ${STM_LIB}/${MCU_FAMILY}_HAL_Driver)

    add_stm32_target(LIB
        TARGET ${PREFIX}_startup
        LIBTYPE OBJECT
        FILES ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/startup_${MCU_LOWER}.s)

    add_stm32_target(LIB
        TARGET ${PREFIX}_CMSIS
        LIBTYPE STATIC
        FILES ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
    target_include_directories(${PREFIX}_CMSIS
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Include
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)

    function(add_hallib libname)
        set(sources "")
        foreach(f ${ARGN})
            list(APPEND sources "${HAL_PATH}/Src/${MCU_FAMILY_LOWER}_${f}")
        endforeach(f)
        add_stm32_target(LIB
            TARGET ${PREFIX}_${libname}
            LIBTYPE STATIC
            FILES ${sources})
        target_include_directories(${PREFIX}_${libname} SYSTEM PUBLIC ${HAL_PATH}/Inc)
        target_link_libraries(${PREFIX}_${libname} PUBLIC ${PREFIX}_CMSIS)
        target_compile_options(${PREFIX}_${libname} PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
    endfunction()

    add_hallib(HAL hal.c hal_rcc.c hal_cortex.c)
    add_hallib(LL ll_utils.c ll_rcc.c)
    add_hallib(LL_Adc ll_adc.c)
    add_hallib(LL_Dma ll_dma.c)
    add_hallib(LL_Exti ll_exti.c)
    add_hallib(LL_Gpio ll_gpio.c)
    add_hallib(LL_Spi ll_spi.c)
    add_hallib(LL_Tim ll_tim.c)
    add_hallib(LL_Usart ll_usart.c)
        target_link_libraries(${PREFIX}_LL_Usart PUBLIC ${PREFIX}_LL)
    add_hallib(HAL_CRC hal_crc.c hal_crc_ex.c)

    # Function 2
    FetchContent_Declare(
        function2
        GIT_REPOSITORY https://github.com/Naios/function2.git
        GIT_TAG        4.0.0
        )
    FetchContent_GetProperties(function2)
    if(NOT function2_POPULATED)
        FetchContent_Populate(function2)
        add_subdirectory(${function2_SOURCE_DIR} ${function2_BINARY_DIR})
    endif()

    # Printf
    FetchContent_Declare(
    printf
    GIT_REPOSITORY https://github.com/mpaland/printf.git
    GIT_TAG        v4.0.0
    )
    FetchContent_GetProperties(printf)
    if(NOT printf_POPULATED)
        FetchContent_Populate(printf)
        add_stm32_target(LIB
            TARGET ${PREFIX}_printf
            LIBTYPE STATIC
            FILES  ${printf_SOURCE_DIR}/printf.c)
        target_include_directories(${PREFIX}_printf PUBLIC ${printf_SOURCE_DIR})
    endif()

    # STM32CXX
    file(GLOB_RECURSE SRC $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src/*.cpp src/*.c)
    file(GLOB_RECURSE IRQ_SRC $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src/*.irq.cpp src/*.irq.c)
    list(REMOVE_ITEM SRC ${IRQ_SRC})

    # We have to separate IRQ into a special library which is linked as whole
    # archive to achieve weak symbol overriding
    add_stm32_target(LIB
        TARGET ${PREFIX}_stm32cxx_irq
        LIBTYPE STATIC
        FILES ${IRQ_SRC})
    target_compile_options(${PREFIX}_stm32cxx_irq PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
    set_property(TARGET ${PREFIX}_stm32cxx_irq PROPERTY CXX_STANDARD 17)
    target_include_directories(${PREFIX}_stm32cxx_irq PUBLIC $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src)
    target_link_libraries(${PREFIX}_stm32cxx_irq PUBLIC ${PREFIX}_printf
        ${PREFIX}_HAL ${PREFIX}_LL
        ${PREFIX}_LL_Adc ${PREFIX}_LL_Dma ${PREFIX}_LL_Exti ${PREFIX}_LL_Gpio
        ${PREFIX}_LL_Spi ${PREFIX}_LL_Usart)
    # Use this to force linking stm32cxx_irq as a whole archive to override the
    # weak symbols
    target_link_options(${PREFIX}_stm32cxx_irq PUBLIC -Wl,--whole-archive $<TARGET_FILE:${PREFIX}_stm32cxx_irq> -Wl,--no-whole-archive)

    add_stm32_target(LIB
        TARGET ${PREFIX}_stm32cxx
        LIBTYPE STATIC
        FILES ${SRC}
              ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/startup_${MCU_LOWER}.s)
    target_compile_options(${PREFIX}_stm32cxx PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
    set_property(TARGET ${PREFIX}_stm32cxx PROPERTY CXX_STANDARD 17)
    target_link_libraries(${PREFIX}_stm32cxx PRIVATE
        ${PREFIX}_stm32cxx_irq)
    target_link_libraries(${PREFIX}_stm32cxx PUBLIC
        function2 ${PREFIX}_printf
        ${PREFIX}_HAL ${PREFIX}_LL
        ${PREFIX}_LL_Adc ${PREFIX}_LL_Dma ${PREFIX}_LL_Exti ${PREFIX}_LL_Gpio
        ${PREFIX}_LL_Spi ${PREFIX}_LL_Usart ${PREFIX}_LL_Tim ${PREFIX}_HAL_CRC)
    target_include_directories(${PREFIX}_stm32cxx PUBLIC $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src)
    target_include_directories(${PREFIX}_stm32cxx SYSTEM PUBLIC ${HAL_PATH}/Inc)

    # There is a pooled memory functionality in the library, if no values are set,
    # define one
    if(NOT DEFINED MEMORY_BLOCK_SIZE)
        set(MEMORY_BLOCK_SIZE 1024)
    endif()

    if(NOT DEFINED MEMORY_BLOCK_COUNT)
        set(MEMORY_BLOCK_COUNT 10)
    endif()

    target_compile_options(${PREFIX}_stm32cxx PUBLIC -DMEMORY_BLOCK_SIZE=${MEMORY_BLOCK_SIZE} -DMEMORY_BLOCK_COUNT=${MEMORY_BLOCK_COUNT})
    target_compile_options(${PREFIX}_stm32cxx_irq PUBLIC -DMEMORY_BLOCK_SIZE=${MEMORY_BLOCK_SIZE} -DMEMORY_BLOCK_COUNT=${MEMORY_BLOCK_COUNT})
endfunction()