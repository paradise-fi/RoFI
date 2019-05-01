
if(${MCU} STREQUAL "STM32G071xx")
    add_definitions(-D${MCU})
    set(MCU_FAMILY STM32G0xx)
    set(MCU_ARCH cortex-m0plus)
    set(MCU_FLOAT_ABI soft)
else()
    message(FATAL_ERROR "Unsupported MCU '${MCU}' specified." )
endif()