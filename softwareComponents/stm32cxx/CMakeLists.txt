cmake_minimum_required(VERSION 3.12)

include(FetchContent)

function(setup_stm32cxx PREFIX MCU)
    if (NOT TARGET ${PREFIX}_CUBE)
        setup_stm32cube(${PREFIX} ${MCU})
    endif()

    setup_mcu_details(${MCU})
    # STM32 HAL and CMSIS
    string(TOLOWER ${MCU} MCU_LOWER)
    string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)

    get_property(STM_LIB
        TARGET ${PREFIX}_CUBE
        PROPERTY STM_LIB)
    get_property(HAL_PATH
        TARGET ${PREFIX}_CUBE
        PROPERTY HAL_PATH)

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
    endif()
    add_library(${PREFIX}_printf INTERFACE)
    target_include_directories(${PREFIX}_printf INTERFACE ${printf_SOURCE_DIR})
    target_sources(${PREFIX}_printf INTERFACE ${printf_SOURCE_DIR}/printf.c)

    # STM32CXX
    function(add_sublib NAME)
        cmake_parse_arguments(A "" "" "SRC_GLOB;PORT_GLOB;LINK_HAL;LINK" ${ARGN})

        list(TRANSFORM A_SRC_GLOB
             PREPEND $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src/)
        list(TRANSFORM A_PORT_GLOB
             PREPEND $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/port/${MCU_FAMILY_LOWER}/)
        list(TRANSFORM A_LINK_HAL
             PREPEND ${PREFIX}_)

        file(GLOB_RECURSE L_SRC ${A_SRC_GLOB} ${A_PORT_GLOB})

        add_library(${PREFIX}_stm32cxx_${NAME} INTERFACE)
        target_sources(${PREFIX}_stm32cxx_${NAME} INTERFACE ${L_SRC})
        target_link_libraries(${PREFIX}_stm32cxx_${NAME} INTERFACE
            ${PREFIX}_stm32cxx ${A_LINK_HAL} ${LINK})
    endfunction()

    add_library(${PREFIX}_stm32cxx INTERFACE)
    target_compile_options(${PREFIX}_stm32cxx INTERFACE
        -DUSE_FULL_LL_DRIVER
        -DUSE_HAL_DRIVER
        -D${MCU_FAMILY})
    target_link_libraries(${PREFIX}_stm32cxx INTERFACE
        function2 ${PREFIX}_printf
        ${PREFIX}_HAL ${PREFIX}_LL ${PREFIX}_HAL_Gpio)
    target_include_directories(${PREFIX}_stm32cxx INTERFACE
        $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/src
        $ENV{ROFI_ROOT}/softwareComponents/stm32cxx/port/${MCU_FAMILY_LOWER})

    add_sublib(uart
        SRC_GLOB uart.cpp
        PORT_GLOB uart*.cpp
        LINK_HAL LL_Usart LL_Gpio)
    add_sublib(spi
        SRC_GLOB spi.cpp
        PORT_GLOB spi.cpp
        LINK_HAL LL_Spi LL_Gpio)
    add_sublib(gpio
        SRC_GLOB gpio*.cpp
        PORT_GLOB gpio*.cpp
        LINK_HAL LL_Gpio LL_Exti)
    add_sublib(dma
        SRC_GLOB dma.cpp
        PORT_GLOB dma*.cpp
        LINK_HAL LL_Dma)
    add_sublib(adc
        SRC_GLOB adc.cpp
        PORT_GLOB adc.cpp
        LINK_HAL LL_Adc)
    add_sublib(crc
        LINK_HAL HAL_CRC)
    add_sublib(timer
        LINK_HAL LL_Tim)
    add_sublib(system
        SRC_GLOB system/assert.cpp system/system.cpp
        LINK_HAL stm32cxx_dma)
endfunction()
