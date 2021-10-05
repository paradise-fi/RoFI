#include <drivers/clock.hpp>

#ifdef STM32F0xx

#include <stm32f0xx_ll_rcc.h>
#include <stm32f0xx_ll_system.h>
#include <stm32f0xx_ll_cortex.h>
#include <stm32f0xx_ll_utils.h>

void setupSystemClock() {
    LL_FLASH_SetLatency( LL_FLASH_LATENCY_0 );
    LL_RCC_HSI_Enable();
    while( LL_RCC_HSI_IsReady() != 1 );

    LL_RCC_HSI_SetCalibTrimming( 16 );
    LL_RCC_HSI14_Enable();
    while( LL_RCC_HSI14_IsReady() != 1 );

    LL_RCC_HSI14_SetCalibTrimming( 16 );
    LL_RCC_PLL_ConfigDomain_SYS( LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_4 );
    LL_RCC_PLL_Enable();
    while( LL_RCC_PLL_IsReady() != 1 );

    LL_RCC_SetAHBPrescaler( LL_RCC_SYSCLK_DIV_1 );
    LL_RCC_SetAPB1Prescaler( LL_RCC_APB1_DIV_1 );
    LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_PLL );
    while( LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL );

    LL_Init1msTick( 16000000 );
    LL_SYSTICK_SetClkSource( LL_SYSTICK_CLKSOURCE_HCLK );
    LL_SetSystemCoreClock( 16000000 );
    LL_RCC_HSI14_EnableADCControl();
    LL_RCC_SetUSARTClockSource( LL_RCC_USART1_CLKSOURCE_PCLK1 );
    LL_RCC_SetI2CClockSource( LL_RCC_I2C1_CLKSOURCE_HSI );
}

#endif