macro(setup_mcu_details MCU)
    if(${MCU} MATCHES "STM32G071.*")
        set(MCU_FAMILY STM32G0xx)
        set(MCU_ARCH cortex-m0plus)
        set(MCU_FLOAT_ABI soft)
    elseif(${MCU} MATCHES "STM32G4.*")
        set(MCU_FAMILY STM32G4xx)
        set(MCU_ARCH cortex-m4)
        set(MCU_FLOAT_ABI hard)
        set(MCU_FPU fpv4-sp-d16)
    elseif(${MCU} STREQUAL "STM32F030x8")
        set(MCU_FAMILY STM32F0xx)
        set(MCU_ARCH cortex-m0)
        set(MCU_FLOAT_ABI soft)
    elseif(${MCU} MATCHES "STM32F41.*")
        set(MCU_FAMILY STM32F4xx)
        set(MCU_ARCH cortex-m4)
        set(MCU_FLOAT_ABI hard)
        set(MCU_FPU fpv4-sp-d16)
    else()
        message(FATAL_ERROR "Unsupported MCU '${MCU}' specified." )
    endif()

    if((${MCU_ARCH} STREQUAL cortex-m0) OR (${MCU_ARCH} STREQUAL cortex-m0plus))
        set(MCU_RTOS_FLAVOR ARM_CM0)
    elseif(${MCU_ARCH} STREQUAL cortex-m4)
        set(MCU_RTOS_FLAVOR ARM_CM4F)
    endif()
endmacro()

set(IMG_DIR ${CMAKE_BINARY_DIR}/img)
file(MAKE_DIRECTORY "${IMG_DIR}")


function(add_stm32_compiler_flags)
    cmake_parse_arguments(A "" "TARGET;MCU;MCU_SPEC" "" ${ARGN})

    setup_mcu_details(${A_MCU})

    set(DMCU_SPEC "")
    if (NOT "${A_MCU_SPEC} " STREQUAL " ")
        set(DMCU_SPEC "-D${A_MCU_SPEC}")
    endif()

    set(BUILD_FLAGS "-D${A_MCU} -D${MCU_FAMILY} ${DMCU_SPEC}\
                     -mcpu=${MCU_ARCH} \
                     -mthumb -mfloat-abi=${MCU_FLOAT_ABI} \
                     -ffunction-sections -fdata-sections -g \
                     -fno-common -fmessage-length=0")
    if (MCU_FLOAT_ABI STREQUAL hard)
        set(BUILD_FLAGS "${BUILD_FLAGS} -mfpu=${MCU_FPU}")
        target_link_options(${A_TARGET} PUBLIC "-mfloat-abi=${MCU_FLOAT_ABI}" "-mfpu=${MCU_FPU}")
    endif()

    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gdwarf-2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gdwarf-2")
    endif()

    set_target_properties(${A_TARGET} PROPERTIES COMPILE_FLAGS ${BUILD_FLAGS})
    set_target_properties(${A_TARGET} PROPERTIES LINKER_LANGUAGE CXX)
    target_link_options(${A_TARGET} PUBLIC "-mthumb" "-mcpu=${MCU_ARCH}")
endfunction()

function(add_stm32_target)

    cmake_parse_arguments(A "EXECUTABLE;LIB" "TARGET;MCU;MCU_SPEC;LINKER_SCRIPT;LIBTYPE" "FILES" ${ARGN})

    if ("${A_EXECUTABLE}")
        add_executable(${A_TARGET} ${A_FILES})
    elseif("${A_LIB}")
        add_library(${A_TARGET} ${A_LIBTYPE} ${A_FILES})
    else()
        message(FATAL_ERROR "You have to specify either EXECUTABLE or a LIB")
    endif()

    add_stm32_compiler_flags(
        TARGET ${A_TARGET}
        MCU ${A_MCU}
        MCU_SPEC ${A_MCU_SPEC})
    target_link_options(
        ${A_TARGET} PUBLIC "-Wl,-Map=control.map,--cref"
        "-Wl,--print-memory-usage" "-funwind-tables" "-fasynchronous-unwind-tables"
        "--specs=nosys.specs" "-Wl,--gc-sections" -lc -lm -lnosys)
    if (NOT "${A_LINKER_SCRIPT}" STREQUAL "")
        target_link_options(${A_TARGET} PUBLIC "-T${A_LINKER_SCRIPT}")
    endif()

    if ("${A_EXECUTABLE}")
        target_compile_options(${A_TARGET} PRIVATE
            $<$<COMPILE_LANGUAGE:C>:${C_DEFS} ${C_FLAGS}>
            $<$<COMPILE_LANGUAGE:CXX>:${CXX_DEFS} ${CXX_FLAGS}>
            $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp ${ASM_FLAGS}>
        )
        set(HEX_FILE ${IMG_DIR}/${A_TARGET}.hex)
        add_custom_command(
            TARGET ${A_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${A_TARGET}> ${HEX_FILE}
            COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")
    endif()
endfunction()
