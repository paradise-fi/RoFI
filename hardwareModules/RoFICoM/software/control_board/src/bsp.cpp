#include "bsp.hpp"

#include "configuration.hpp"
#include "motor.hpp"

#include <optional>

#include <stm32g0xx_hal.h>
#include <stm32g0xx_ll_rcc.h>
#include <stm32g0xx_ll_system.h>
#include <stm32g0xx_ll_cortex.h>
#include <stm32g0xx_ll_utils.h>
#include <stm32g0xx_ll_i2c.h>

namespace {
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

        LL_Init1msTick( 64000000 );

        LL_SYSTICK_SetClkSource( LL_SYSTICK_CLKSOURCE_HCLK );
        /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
        LL_SetSystemCoreClock( 64000000 );
        LL_RCC_SetUSARTClockSource( LL_RCC_USART1_CLKSOURCE_PCLK1 );
        LL_RCC_SetUSARTClockSource( LL_RCC_USART2_CLKSOURCE_PCLK1 );
    }
}

namespace bsp {
    const Gpio::Pin connectorSenseA = { 13, GPIOC };
    const Gpio::Pin connectorSenseB = { 14, GPIOC };
    const Gpio::Pin internalSwitchPin = { 3, GPIOD };
    const Gpio::Pin internalVoltagePin = { 0, GPIOA };
    const Gpio::Pin internalCurrentPin = { 0, GPIOB };
    const Gpio::Pin externalSwitchPin = { 2, GPIOD };
    const Gpio::Pin externalVoltagePin = { 7, GPIOA };
    const Gpio::Pin externalCurrentPin = { 1, GPIOB };
    
    const Gpio::Pin spiCSPin = { 4, GPIOA };
    const Gpio::Pin lidarEnablePin = { 0, GPIOD };
    const Gpio::Pin lidarIRQPin = { 1, GPIOD };

    const std::array< Gpio::Pin, 10 > posPins = {
        Gpio::Pin {  2, GPIOB },
        Gpio::Pin { 10, GPIOB },
        Gpio::Pin { 11, GPIOB },
        Gpio::Pin { 12, GPIOB },
        Gpio::Pin { 13, GPIOB },
        Gpio::Pin { 14, GPIOB },
        Gpio::Pin { 15, GPIOB },
        Gpio::Pin {  6, GPIOC },
        Gpio::Pin {  7, GPIOC },
        Gpio::Pin { 10, GPIOA },
    };

    std::optional< Timer > motorTimer;
    std::optional< Timer::Pwm > motorPwm;
    std::optional< Motor > sliderMotor;
    std::optional< Spi > moduleComm;
    std::optional< Uart > connectorComm;


    std::optional< I2C > i2c;

    void setupBoard() {
        setupSystemClock();
        SystemCoreClockUpdate();

        Adc1.setup();
        Adc1.calibrate();

        motorTimer = Timer( TIM1, FreqAndRes( 1000, 2000 ) );

        motorPwm = motorTimer->pwmChannel( LL_TIM_CHANNEL_CH1 );
        motorPwm->attachPin( GpioA[ 8 ] );
        motorTimer->enable();

        sliderMotor = Motor( motorPwm.value(), GpioA[ 9 ] );
        sliderMotor->enable();
        sliderMotor->set( 0 );
        
        moduleComm = Spi( SPI1,
            Slave(),
            MisoOn( GpioA[ 6 ] ),
            SckOn( GpioA[ 5 ] ),
            CsOn( spiCSPin )
        );

        connectorComm = Uart( USART2,
            Baudrate( cfg::TRANSMIT_BAUDRATE ),
            TxOn( GpioA[ 2 ] ),
            RxOn( GpioA[ 3 ] ),
            UartOversampling( 8 ) );
        connectorComm->enable();

        i2c = I2C( I2C2, SdaPin( GpioA[12] ), SclPin( GpioA[11] ), I2C::SpeedMode::Standard );
    }


} // namespace bsp

Dbg& dbgInstance() {
    static Dbg inst(
        USART1, Dma::allocate( DMA1, 1 ), Dma::allocate( DMA1, 2 ),
        TxOn( GpioB[ 6 ] ),
        RxOn( GpioB[ 7 ] ),
        Baudrate( cfg::DBG_BAUDRATE ) );
    return inst;
}
