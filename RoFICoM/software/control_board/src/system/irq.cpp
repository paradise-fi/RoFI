
#include <stm32g0xx_hal.h>
#include <system/dbg.hpp>

extern "C" void HardFault_Handler() {
    Dbg::error( "HardFault occured" );
    while ( true );
}

extern "C" void NMI_Handler() {
    Dbg::error( "NMI occured" );
    while ( true );
}

extern "C" void SysTick_Handler() {
    HAL_IncTick();
}