#include <drivers/clock.hpp>

#ifdef STM32G0xx

#include <stm32g0xx_ll_rcc.h>
#include <stm32g0xx_ll_system.h>
#include <stm32g0xx_ll_cortex.h>
#include <stm32g0xx_ll_utils.h>

void setupSystemClock() {
    LL_FLASH_SetLatency( LL_FLASH_LATENCY_2 );
    LL_RCC_HSI_Enable();
    while( LL_RCC_HSI_IsReady() != 1 );

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS( LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2 );
    LL_RCC_PLL_Enable();
    LL_RCC_PLL_EnableDomain_SYS();
    while( LL_RCC_PLL_IsReady() != 1 );

    /* Set AHB prescaler*/
    LL_RCC_SetAHBPrescaler( LL_RCC_SYSCLK_DIV_1 );

    /* Sysclk activation on the main PLL */
    LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_PLL );
    while( LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL );

    /* Set APB1 prescaler*/
    LL_RCC_SetAPB1Prescaler( LL_RCC_APB1_DIV_1 );

    LL_Init1msTick(64000000);

    LL_SYSTICK_SetClkSource( LL_SYSTICK_CLKSOURCE_HCLK );
    /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
    LL_SetSystemCoreClock( 64000000 );
    LL_RCC_SetUSARTClockSource( LL_RCC_USART1_CLKSOURCE_PCLK1 );
    LL_RCC_SetUSARTClockSource( LL_RCC_USART2_CLKSOURCE_PCLK1 );
}

#endif
