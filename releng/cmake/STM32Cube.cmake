cmake_minimum_required(VERSION 3.11)

include(FetchContent)

function(cube_resolve_startup MCU OUTPUT)
    # Some MCUs share startup script, rename and then follow a common pattern
    if (${MCU} STREQUAL "STM32F411xC")
        set(MCU "STM32F411xE")
    endif()

    string(TOLOWER ${MCU} MCU_LOWER)
    set(${OUTPUT} startup_${MCU_LOWER}.s)
endfunction()

function(setup_stm32cube PREFIX MCU)

    setup_mcu_details(${MCU})

    enable_language(C CXX ASM)

    # STM32 HAL and CMSIS
    string(TOLOWER ${MCU} MCU_LOWER)
    string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)

    # Fetch HAL libraries
    if(${MCU_FAMILY} STREQUAL "STM32F0xx")
        FetchContent_Declare(
            hal_f0
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF0.git
            GIT_TAG        v1.11.0
        )
        set(hal hal_f0)
    elseif(${MCU_FAMILY} STREQUAL "STM32F4xx")
        FetchContent_Declare(
            hal_f4
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF4.git
            GIT_TAG        v1.26.2
        )
        set(hal hal_f4)
    elseif(${MCU_FAMILY} STREQUAL "STM32G0xx")
        FetchContent_Declare(
            hal_g0
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeG0.git
            GIT_TAG        v1.3.0
        )
        set(hal hal_g0)
    else()
        message(FATAL_ERROR "Unsupported MCU family '${MCU_FAMILY}' specified." )
    endif()

    FetchContent_GetProperties("${hal}")
    if(NOT "${${hal}_POPULATED}")
        FetchContent_Populate(${hal})

        # Turn config template into config
        file(GLOB_RECURSE cfgFile "${${hal}_SOURCE_DIR}/Drivers/*_conf_template.h")
        foreach(f ${cfgFile})
            string(REGEX REPLACE "_template\.h$" ".h" newF ${f})
            file(RENAME ${f} ${newF})
        endforeach(f)
    endif()

    set(STM_LIB ${${hal}_SOURCE_DIR}/Drivers)
    set(HAL_PATH ${STM_LIB}/${MCU_FAMILY}_HAL_Driver)

    cube_resolve_startup(${MCU} STARTUP_SCRIPT)
    add_stm32_target(LIB
        TARGET ${PREFIX}_startup
        MCU ${MCU}
        LIBTYPE OBJECT
        FILES ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/${STARTUP_SCRIPT})

    add_stm32_target(LIB
        TARGET ${PREFIX}_CMSIS
        MCU ${MCU}
        LIBTYPE STATIC
        FILES ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
    target_include_directories(${PREFIX}_CMSIS
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Include
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)

    function(add_hallib libname)
        cmake_parse_arguments(A "" "" "REQUIRED;OPTIONAL" ${ARGN})
        set(sources "")
        foreach(f ${A_REQUIRED})
            list(APPEND sources "${HAL_PATH}/Src/${MCU_FAMILY_LOWER}_${f}")
        endforeach(f)
        foreach(f ${A_OPTIONAL})
            if(EXISTS "${HAL_PATH}/Src/${MCU_FAMILY_LOWER}_${f}")
                list(APPEND sources "${HAL_PATH}/Src/${MCU_FAMILY_LOWER}_${f}")
            endif()
        endforeach(f)
        add_stm32_target(LIB
            TARGET ${PREFIX}_${libname}
            MCU ${MCU}
            LIBTYPE STATIC
            FILES ${sources})
        target_include_directories(${PREFIX}_${libname} SYSTEM PUBLIC ${HAL_PATH}/Inc)
        target_link_libraries(${PREFIX}_${libname} PUBLIC ${PREFIX}_CMSIS)
        target_compile_options(${PREFIX}_${libname} PUBLIC -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -D${MCU_FAMILY})
    endfunction()

    add_hallib(HAL
        REQUIRED hal.c hal_rcc.c hal_cortex.c)
    add_hallib(LL
        REQUIRED ll_utils.c ll_rcc.c)
    add_hallib(LL_Adc
        REQUIRED ll_adc.c)
    add_hallib(LL_Dma
        REQUIRED ll_dma.c)
    add_hallib(LL_Exti
        REQUIRED ll_exti.c)
    add_hallib(LL_Gpio
        REQUIRED ll_gpio.c)
    add_hallib(LL_Spi
        REQUIRED ll_spi.c)
    add_hallib(LL_Tim
        REQUIRED ll_tim.c)
    add_hallib(LL_Usart
        REQUIRED ll_usart.c)
        target_link_libraries(${PREFIX}_LL_Usart PUBLIC ${PREFIX}_LL)
    add_hallib(HAL_CRC
        REQUIRED hal_crc.c
        OPTIONAL hal_crc_ex.c)

    set_property(TARGET ${PREFIX}_CMSIS
        PROPERTY STM_LIB ${STM_LIB})
    set_property(TARGET ${PREFIX}_CMSIS
        PROPERTY HAL_PATH ${HAL_PATH})



    set(fp ${STM_LIB}/../Middlewares/Third_Party/FreeRTOS/Source)
    file(GLOB freertos_src
        ${fp}/CMSIS_RTOS/*.c
        ${fp}/portable/GCC/${MCU_RTOS_FLAVOR}/*.c
        ${fp}/*.c
        ${fp}/portable/MemMang/heap_4.c # Use heap that supports freeing
    )
    add_library(${PREFIX}_freertos INTERFACE)
    target_sources(${PREFIX}_freertos INTERFACE ${freertos_src})
    target_include_directories(${PREFIX}_freertos INTERFACE
        ${fp}/CMSIS_RTOS
        ${fp}/portable/GCC/${MCU_RTOS_FLAVOR}
        ${fp}/include)
endfunction()