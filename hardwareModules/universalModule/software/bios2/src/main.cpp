// #include <FreeRTOS.h>

#include <unwind.h>

#include <drivers/uart.hpp>
#include <drivers/usb.hpp>
#include <drivers/usb_cdc.hpp>
#include <system/dbg.hpp>
#include <system/defer.hpp>
#include <array>
#include <cstring>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_ll_rcc.h>
#include <stm32f4xx_ll_system.h>
#include <stm32f4xx_ll_cortex.h>
#include <stm32f4xx_ll_utils.h>
#include <stm32f4xx_ll_pwr.h>

#include "board.hpp"
#include "espTunnel.hpp"
#include "powerManagement.hpp"

void Error_Handler() { abort(); }

void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_3)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_12, 96, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_12, 96, LL_RCC_PLLQ_DIV_4);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_2);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(96000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_SetTIMPrescaler(LL_RCC_TIM_PRESCALER_TWICE);
}

void setupSystemClock() {
    return SystemClock_Config();

    LL_FLASH_SetLatency( LL_FLASH_LATENCY_0 );
    while( LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0 );
    LL_PWR_SetRegulVoltageScaling( LL_PWR_REGU_VOLTAGE_SCALE1 );
    LL_RCC_HSE_Enable();

    // Wait till HSE is ready
    while( LL_RCC_HSE_IsReady() != 1 );

    // Wait till HSI is ready
    while( LL_RCC_HSI_IsReady() != 1 );
    LL_RCC_PLL_ConfigDomain_48M( LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_12, 72, LL_RCC_PLLQ_DIV_3 );
    LL_RCC_PLL_Enable();

    // Wait till PLL is ready
    while( LL_RCC_PLL_IsReady() != 1 );
    LL_RCC_SetAHBPrescaler( LL_RCC_SYSCLK_DIV_1 );
    LL_RCC_SetAPB1Prescaler( LL_RCC_APB1_DIV_1 );
    LL_RCC_SetAPB2Prescaler( LL_RCC_APB2_DIV_1 );
    LL_RCC_SetSysClkSource( LL_RCC_SYS_CLKSOURCE_HSE );

    // Wait till System clock is ready
    while( LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE );
    LL_SetSystemCoreClock( 24000000 );

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
        Baudrate( 921600 ) );
    return inst;
}

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();
    HAL_Init();
    NVIC_SetPriority( SysTick_IRQn, 0 );

    PowerManagement::instance().setup();

    EspTunnelManager::instance().setupUsbConfiguration();
    EspTunnelManager::instance().setupTunnel();

    UsbDevice::instance().enable();
    Dbg::error("Starting");

    int i = 0;
    std::function< void(void) > keepAlive = [&](){
        if ( i < 2 ) {
            Dbg::error("Buzz!");
            PowerManagement::instance().beepStart( 1500, 255 );
        }
        else
            PowerManagement::instance().beepStop();
        // PowerManagement::instance().enableUsbToBus( i % 12 < 6 );
        // PowerManagement::instance().enableCharging( i % 6 < 3 );
        float usbV = PowerManagement::instance().getUsbVoltage();
        float busV = PowerManagement::instance().getBusVoltage();
        Dbg::info( "Alive! %d, %f V, %f V", i % 12, usbV, busV );
        i++;

        Defer::schedule( 1000, keepAlive );
    };
    keepAlive();

    while ( true ) {
        Defer::run();
    }
}