macro(setup_mcu_details MCU)
    if(${MCU} STREQUAL "STM32G071xx")
        add_definitions(-D${MCU})
        set(MCU_FAMILY STM32G0xx)
        set(MCU_ARCH cortex-m0plus)
        set(MCU_FLOAT_ABI soft)
    elseif(${MCU} STREQUAL "STM32F030x8")
        add_definitions(-D${MCU})
        set(MCU_FAMILY STM32F0xx)
        set(MCU_ARCH cortex-m0)
        set(MCU_FLOAT_ABI soft)
    else()
        message(FATAL_ERROR "Unsupported MCU '${MCU}' specified." )
    endif()
endmacro()


function(add_stm32_compiler_flags)
    cmake_parse_arguments(A "" "TARGET;MCU" "" ${ARGN})

    setup_mcu_details(${MCU})

    set(BUILD_FLAGS "-mcpu=${MCU_ARCH} \
                     -mthumb -mfloat-abi=${MCU_FLOAT_ABI} \
                     -ffunction-sections -fdata-sections -g \
                     -fno-common -fmessage-length=0 \
                     -fno-exceptions -fno-rtti")
    if (MCU_FLOAT_ABI STREQUAL hard)
        set(BUILD_FLAGS "${BUILD_FLAGS} -mfpu=${MCU_FPU}")
    endif ()

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gdwarf-2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gdwarf-2")
    endif()

    set_target_properties(${A_TARGET} PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})
    set_target_properties(${A_TARGET} PROPERTIES LINKER_LANGUAGE CXX)
endfunction()

function(add_stm32_target)

    cmake_parse_arguments(A "EXECUTABLE;LIB" "TARGET;MCU;LINKER_SCRIPT;LIBTYPE" "FILES" ${ARGN})

    if ("${A_EXECUTABLE}")
        add_executable(${A_TARGET} ${A_FILES})
    elseif("${A_LIB}")
        add_library(${A_TARGET} ${A_LIBTYPE} ${A_FILES})
    else()
        message(FATAL_ERROR "You have to specify either EXECUTABLE or a LIB")
    endif()
    add_stm32_compiler_flags(
        TARGET ${A_TARGET}
        MCU ${MCU})
    target_link_options(
        ${A_TARGET} PRIVATE "-Wl,-Map=control.map,--cref"
        "-Wl,--print-memory-usage" "-funwind-tables" "-fasynchronous-unwind-tables"
        "--specs=nosys.specs" "-Wl,--gc-sections" -lc -lm -lnosys
        "-T" "${A_LINKER_SCRIPT}")

    if ("${A_EXECUTABLE}")
        set(HEX_FILE ${PROJECT_BINARY_DIR}/${A_TARGET}.hex)
        set(BIN_FILE ${PROJECT_BINARY_DIR}/${A_TARGET}.bin)
        add_custom_command(
            TARGET ${A_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${A_TARGET}> ${HEX_FILE}
            COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${A_TARGET}>
                    ${BIN_FILE}
            COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
    endif()
endfunction()
