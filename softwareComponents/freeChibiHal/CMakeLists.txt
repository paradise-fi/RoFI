cmake_minimum_required(VERSION 3.11)

include(FetchContent)

FetchContent_Declare(
    chibi
    URL https://github.com/ChibiOS/ChibiOS/archive/refs/tags/ver21.6.0.tar.gz
)
FetchContent_GetProperties(chibi)
if(NOT chibi_POPULATED)
    FetchContent_Populate(chibi)
endif()

function(setup_chibiStartup PREFIX MCU)
    setup_mcu_details(${MCU})
    FetchContent_GetProperties(chibi)

    set(lib ${PREFIX}_ChibiStartup)
    set(chibiStartupSrc ${chibi_SOURCE_DIR}/os/common)

    add_library(${lib} INTERFACE)
    target_sources(${lib} INTERFACE
        ${chibiStartupSrc}/startup/ARMCMx/compilers/GCC/vectors.S
        ${chibiStartupSrc}/startup/ARMCMx/compilers/GCC/crt1.c)
    target_include_directories(${lib} INTERFACE
        ${chibiStartupSrc}/portability/GCC
        ${chibiStartupSrc}/startup/ARMCMx/compilers/GCC
        ${chibiStartupSrc}/startup/ARMCMx/devices/${MCU_FAMILY}
        ${chibiStartupSrc}/ext/ST/${MCU_FAMILY}
        ${chibiStartupSrc}/ext/ARM/CMSIS/Core/Include
        )

    if(${MCU_FAMILY} STREQUAL "STM32F4xx")
        target_sources(${lib} INTERFACE
            ${chibiStartupSrc}/startup/ARMCMx/compilers/GCC/crt0_v7m.S)
    else()
        message(FATAL_ERROR "Unsupported MCU family: ${MCU_FAMILY}")
    endif()

    target_link_options(${lib} INTERFACE
        "-Wl,--library-path=${chibiStartupSrc}/startup/ARMCMx/compilers/GCC/ld"
        "-Wl,--defsym=__main_stack_size__=0x400"
        "-Wl,--defsym=__process_stack_size__=0x400"
        "-T" "${chibiStartupSrc}/startup/ARMCMx/compilers/GCC/ld/${MCU}.ld")

    get_target_property(X ${lib} INTERFACE_SOURCES)
endfunction()

function(setup_chibiHal PREFIX MCU)
    setup_mcu_details(${MCU})

    FetchContent_GetProperties(chibi)

    setup_chibiStartup(${PREFIX} ${MCU})

    set(ChibiHalSrc ${chibi_SOURCE_DIR}/os/hal)

    file(GLOB ChibiOSAL_src $ENV{ROFI_ROOT}/softwareComponents/freeChibiHal/src/*.c)
    file(GLOB ChibiPort_src
        ${ChibiHalSrc}/src/*.c
        ${ChibiHalSrc}/ports/common/ARMCMx/*.c
        ${ChibiHalSrc}/ports/STM32/${MCU_FAMILY}/*.c)

    add_library(${PREFIX}_ChibiBase INTERFACE)
    target_sources(${PREFIX}_ChibiBase INTERFACE
        ${ChibiOSAL_src}
        ${ChibiPort_src})
    target_include_directories(${PREFIX}_ChibiBase INTERFACE
        $ENV{ROFI_ROOT}/softwareComponents/freeChibiHal/src/
        ${ChibiHalSrc}/include

        ${ChibiHalSrc}/ports/common/ARMCMx
        ${ChibiHalSrc}/ports/STM32/${MCU_FAMILY})

    add_library(${PREFIX}_ChibiFull INTERFACE)
    target_link_libraries(${PREFIX}_ChibiFull INTERFACE ${PREFIX}_ChibiBase)

    function(add_driver name)
        set(libName ${PREFIX}_Chibi${name})
        string(REGEX REPLACE "v[1-9]" "" genericName ${name})
        file(GLOB src
            ${ChibiHalSrc}/ports/STM32/LLD/${name}/*.c
            ${ChibiHalSrc}/ports/STM32/LLD/${genericName}/*.c)
        add_library(${libName} INTERFACE)
        target_sources(${libName} INTERFACE
            ${src})
        target_include_directories(${libName} INTERFACE
            ${ChibiHalSrc}/ports/STM32/LLD/${name}/
            ${ChibiHalSrc}/ports/STM32/LLD/${genericName}/)
        target_link_libraries(${PREFIX}_ChibiFull INTERFACE ${libName})
    endfunction()

    add_driver(ADCv2)
    add_driver(CANv1)
    add_driver(CRYPv1)
    add_driver(DACv1)
    add_driver(DMAv2)
    add_driver(EXTIv1)
    add_driver(GPIOv2)
    add_driver(I2Cv1)
    add_driver(MACv1)
    add_driver(OTGv1)
    add_driver(QUADSPIv1)
    add_driver(RTCv2)
    add_driver(SPIv1)
    add_driver(SDIOv1)
    add_driver(SYSTICKv1)
    add_driver(TIMv1)
    add_driver(USARTv1)
    add_driver(xWDGv1)

endfunction()
