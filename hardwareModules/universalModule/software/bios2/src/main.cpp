// #include <FreeRTOS.h>

#include <unwind.h>

#include <drivers/uart.hpp>
#include <system/dbg.hpp>
#include <array>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_system.h>
#include <stm32f4xx_ll_cortex.h>
#include <stm32f4xx_ll_utils.h>
#include <stm32f4xx_ll_pwr.h>

void setupSystemClock() {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
    while( LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0 );
    LL_PWR_SetRegulVoltageScaling( LL_PWR_REGU_VOLTAGE_SCALE1 );
    LL_RCC_HSE_Enable();

    // Wait till HSE is ready
    while( LL_RCC_HSE_IsReady() != 1 );
    LL_RCC_HSI_SetCalibTrimming( 16 );
    LL_RCC_HSI_Enable();

    // Wait till HSI is ready
    while( LL_RCC_HSI_IsReady() != 1 );
    LL_RCC_PLL_ConfigDomain_48M( LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_12, 72, LL_RCC_PLLQ_DIV_3 );
    LL_RCC_PLL_Enable();

    // Wait till PLL is ready
    while( LL_RCC_PLL_IsReady() != 1 );
    LL_RCC_SetAHBPrescaler( LL_RCC_SYSCLK_DIV_1 );
    LL_RCC_SetAPB1Prescaler( LL_RCC_APB1_DIV_1 );
    LL_RCC_SetAPB2Prescaler( LL_RCC_APB2_DIV_1 );
    LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_HSI );

    // Wait till System clock is ready
    while( LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI );
    LL_SetSystemCoreClock( 16000000 );

    // Update the time base
    if ( HAL_InitTick( TICK_INT_PRIORITY ) != HAL_OK ) {
        assert( false && "Incorrect tick configuration " );
    }
    LL_RCC_SetTIMPrescaler( LL_RCC_TIM_PRESCALER_TWICE );
}


Dbg& dbgInstance() {
    static Dbg inst(
        USART2, Dma::allocate( DMA1, 5 ), Dma::allocate( DMA1, 6 ),
        TxOn( GpioA[ 2 ] ),
        RxOn( GpioA[ 3 ] ),
        Baudrate( 115200 ) );
    return inst;
}

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();
    HAL_Init();


    while ( true ) {
        Dbg::error( "Error!" );
        HAL_Delay( 500 );
        Dbg::info( "Info works!" );
        HAL_Delay( 500 );
    }
}