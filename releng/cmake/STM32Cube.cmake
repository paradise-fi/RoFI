cmake_minimum_required(VERSION 3.11)

include(FetchContent)

function(cube_resolve_startup MCU OUTPUT)
    # Some MCUs share startup script, rename and then follow a common pattern
    if (${MCU} STREQUAL "STM32F411xC")
        set(MCU "STM32F411xE")
    endif()

    string(TOLOWER ${MCU} MCU_LOWER)
    set(${OUTPUT} startup_${MCU_LOWER}.s PARENT_SCOPE)
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
    elseif(${MCU_FAMILY} STREQUAL "STM32G4xx")
        FetchContent_Declare(
            hal_g4
            GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeG4.git
            GIT_TAG        v1.4.0
        )
        set(hal hal_g4)
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
        # Delete the projects directory as it kills most IDEs (multiple copies
        # of the drivers package) and it is not needed for the compilation.
        file(REMOVE_RECURSE "${${hal}_SOURCE_DIR}/Projects")
    endif()

    set(STM_LIB ${${hal}_SOURCE_DIR}/Drivers)
    set(HAL_PATH ${STM_LIB}/${MCU_FAMILY}_HAL_Driver)

    cube_resolve_startup(${MCU} STARTUP_SCRIPT)
    add_library(${PREFIX}_startup INTERFACE)
    target_sources(${PREFIX}_startup INTERFACE
        ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/${STARTUP_SCRIPT})
    set_source_files_properties(
        ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/gcc/${STARTUP_SCRIPT}
        PROPERTIES COMPILE_FLAGS
        "-Wno-all -Wno-extra -Wno-pedantic -Wno-unused-parameter")

    add_stm32_target(LIB
        TARGET ${PREFIX}_CUBE
        MCU ${MCU}
        LIBTYPE STATIC
        FILES ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
    target_include_directories(${PREFIX}_CUBE
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Include
        SYSTEM PUBLIC ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)

    add_library(${PREFIX}_CMSIS INTERFACE)
    target_sources(${PREFIX}_CMSIS INTERFACE
        ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Source/Templates/system_${MCU_FAMILY_LOWER}.c)
    target_include_directories(${PREFIX}_CMSIS
        SYSTEM INTERFACE ${STM_LIB}/CMSIS/Include
        SYSTEM INTERFACE ${STM_LIB}/CMSIS/Device/ST/${MCU_FAMILY}/Include)

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
        add_library(${PREFIX}_${libname} INTERFACE)
        target_sources(${PREFIX}_${libname} INTERFACE ${sources})
        # HAL yields a lot of warnings, ignore them
        foreach(f ${sources})
            set_source_files_properties("${f}" PROPERTIES COMPILE_FLAGS
                "-Wno-all -Wno-extra -Wno-pedantic -Wno-unused-parameter")
        endforeach()
        target_include_directories(${PREFIX}_${libname} SYSTEM INTERFACE ${HAL_PATH}/Inc)
        target_link_libraries(${PREFIX}_${libname} INTERFACE ${PREFIX}_CMSIS)
        target_compile_options(${PREFIX}_${libname} INTERFACE
            -DUSE_FULL_LL_DRIVER
            -DUSE_HAL_DRIVER)
    endfunction()

    add_hallib(HAL
        REQUIRED hal.c hal_rcc.c hal_cortex.c
        OPTIONAL hal_rcc_ex.c)
    add_hallib(HAL_Gpio
        REQUIRED hal_gpio.c)
    add_hallib(HAL_ADC
        REQUIRED hal_adc.c
        OPTIONAL hal_adc_ex.c)
    add_hallib(HAL_CAN
        REQUIRED hal_can.c
        OPTIONAL hal_can_ex.c)
    add_hallib(HAL_CEC
        OPTIONAL hal_cec.c)
    add_hallib(HAL_Cortex
        REQUIRED hal_cortex.c)
    add_hallib(HAL_CRC
        REQUIRED hal_crc.c
        OPTIONAL hal_crc_ex.c)
    add_hallib(HAL_Cryp
        REQUIRED hal_cryp.c
        OPTIONAL hal_crc_ex.c)
    add_hallib(HAL_DAC
        REQUIRED hal_dac.c
        OPTIONAL hal_dac_ex.c)
    add_hallib(HAL_DCMI
        REQUIRED hal_dcmi.c
        OPTIONAL hal_dcmi_ex.c)
    add_hallib(HAL_DMA
        REQUIRED hal_dma.c
        OPTIONAL hal_dma2d.c)
    add_hallib(HAL_Flash
        REQUIRED hal_flash.c hal_flash_ramfunc.c
        OPTIONAL hal_flash_ex.c)
    add_hallib(HAL_Hash
        REQUIRED hal_hash.c)
    add_hallib(HAL_I2C
        REQUIRED hal_i2c.c
        OPTIONAL hal_i2c_ex.c)
    add_hallib(HAL_I2S
        REQUIRED hal_i2s.c
        OPTIONAL hal_i2s_ex.c)
    add_hallib(HAL_RCC
        REQUIRED hal_rcc.c)
    add_hallib(HAL_SPI
        REQUIRED hal_spi.c
        OPTIONAL hal_spi_ex.c)
    add_hallib(HAL_Tim
        REQUIRED hal_tim.c
        OPTIONAL hal_tim_ex.c)
    add_hallib(HAL_UART
        REQUIRED hal_uart.c
        OPTIONAL hal_uart_ex.c)
    add_hallib(HAL_Opamp
        REQUIRED hal_opamp.c
        OPTIONAL hal_opamp_ex.c)
    add_hallib(HAL_Pwr
        REQUIRED hal_pwr.c
        OPTIONAL hal_pwr_ex.c)

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
        REQUIRED ll_tim.c
        OPTIONAL ll_tim_ex.c)
    add_hallib(LL_Usart
        REQUIRED ll_usart.c)
        target_link_libraries(${PREFIX}_LL_Usart INTERFACE ${PREFIX}_LL)
    add_hallib(LL_I2C
        REQUIRED ll_i2c.c)

    set_property(TARGET ${PREFIX}_CUBE
        PROPERTY STM_LIB ${STM_LIB})
    set_property(TARGET ${PREFIX}_CUBE
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
    # FreeRTOS yields warnings, ignore them
    foreach(f ${freertos_src})
        set_source_files_properties("${f}" PROPERTIES COMPILE_FLAGS
            "-Wno-all -Wno-extra -Wno-pedantic -Wno-unused-parameter")
    endforeach()
    target_include_directories(${PREFIX}_freertos INTERFACE
        ${fp}/CMSIS_RTOS
        ${fp}/portable/GCC/${MCU_RTOS_FLAVOR}
        ${fp}/include)
endfunction()
