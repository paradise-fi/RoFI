cmake_minimum_required(VERSION 3.12)

include(FetchContent)

# We need to capture the source directory as it is used in functions invoked
# from other list files
set(STM32CXX_SRC "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "STM32CXX_SRC")

function(setup_stm32cxx PREFIX MCU)
    if (ARGC EQUAL 3)
        set(MCU_SPEC ${ARGV2})
    endif()
    if (NOT TARGET ${PREFIX}_CUBE)
        setup_stm32cube(${PREFIX} ${MCU} ${MCU_SPEC})
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
        GIT_TAG        4.2.2
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

    # Libusb STM32
    FetchContent_Declare(
        libusb_stm32
        GIT_REPOSITORY https://github.com/dmitrystu/libusb_stm32.git
        GIT_TAG        48fc3516278cc2415ebf00bf249125eac92a196c
    )
    FetchContent_GetProperties(libusb_stm32)
    if(NOT libusb_stm32_POPULATED)
        FetchContent_Populate(libusb_stm32)
    endif()
    if (${MCU_FAMILY} STREQUAL "STM32F4xx")
        set(USB_SRC ${libusb_stm32_SOURCE_DIR}/src/usbd_stm32f429_otgfs.c)
        set(USB_DEFS "-DUSBD_SOF_DISABLED")
    elseif (${MCU_FAMILY} STREQUAL "STM32G4xx")
        set(USB_SRC ${libusb_stm32_SOURCE_DIR}/src/usbd_stm32l433_devfs.c
                    ${libusb_stm32_SOURCE_DIR}/src/usbd_stm32l052_devfs_asm.S)
        set(USB_DEFS "-DUSBD_SOF_DISABLED" "-DSTM32G4" "-DUSB_PMASIZE=1024")
    endif()
    # TBA: Add support for other families
    if (NOT "${USB_SRC}" STREQUAL "")
        set(USB_SUPPORTED TRUE)
    endif()

    if ("${USB_SUPPORTED}")
        add_library(${PREFIX}_libusb_stm32 INTERFACE)
        target_include_directories(${PREFIX}_libusb_stm32
            SYSTEM INTERFACE ${libusb_stm32_SOURCE_DIR}/inc)
        target_sources(${PREFIX}_libusb_stm32 INTERFACE
            ${libusb_stm32_SOURCE_DIR}/src/usbd_core.c
            ${USB_SRC})
        foreach(f ${USB_SRC} ${libusb_stm32_SOURCE_DIR}/src/usbd_core.c)
            set_source_files_properties("${f}" PROPERTIES COMPILE_FLAGS
                "-Wno-all -Wno-extra -Wno-pedantic -Wno-unused-parameter")
        endforeach()
        target_compile_definitions(${PREFIX}_libusb_stm32 INTERFACE ${USB_DEFS})
    endif()


    # STM32CXX
    function(add_sublib NAME)
        cmake_parse_arguments(A "" "" "SRC_GLOB;PORT_GLOB;LINK_HAL;LINK" ${ARGN})

        list(TRANSFORM A_SRC_GLOB
             PREPEND ${STM32CXX_SRC}/src/)
        list(TRANSFORM A_PORT_GLOB
             PREPEND ${STM32CXX_SRC}/port/${MCU_FAMILY_LOWER}/)
        list(TRANSFORM A_LINK_HAL
             PREPEND ${PREFIX}_)

        file(GLOB_RECURSE L_SRC ${A_SRC_GLOB} ${A_PORT_GLOB})

        add_library(${PREFIX}_stm32cxx_${NAME} INTERFACE)
        target_sources(${PREFIX}_stm32cxx_${NAME} INTERFACE ${L_SRC})
        target_link_libraries(${PREFIX}_stm32cxx_${NAME} INTERFACE
            ${PREFIX}_stm32cxx ${A_LINK_HAL} ${LINK})
        if (NOT "${MCU_SPEC} " STREQUAL " ")
            target_compile_options(${PREFIX}_stm32cxx_${NAME} INTERFACE -D${MCU_SPEC})
        endif()
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
        ${STM32CXX_SRC}/src
        ${STM32CXX_SRC}/port/${MCU_FAMILY_LOWER})

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
    add_sublib(i2c 
        LINK_HAL LL_I2C)

    if ("${USB_SUPPORTED}")
        add_sublib(usb
            SRC_GLOB drivers/usb.cpp
            PORT_GLOB usb*.cpp
            LINK_HAL libusb_stm32 LL_Gpio)
    endif()
endfunction()
