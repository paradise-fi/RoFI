cmake_minimum_required(VERSION 3.11)

include(FetchContent)

function(setup_stm32cxx PREFIX MCU)
    if (NOT TARGET ${PREFIX}_CMSIS)
        setup_stm32cube(${PREFIX} ${MCU})
    endif()

    setup_mcu_details(${MCU})
    # STM32 HAL and CMSIS
    string(TOLOWER ${MCU} MCU_LOWER)
    string(TOLOWER ${MCU_FAMILY} MCU_FAMILY_LOWER)

    get_property(STM_LIB
        TARGET ${PREFIX}_CMSIS
        PROPERTY STM_LIB)
    get_property(HAL_PATH
        TARGET ${PREFIX}_CMSIS
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
        add_stm32_target(LIB
            TARGET ${PREFIX}_printf
            MCU ${MCU}
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
        MCU ${MCU}
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
        MCU ${MCU}
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
