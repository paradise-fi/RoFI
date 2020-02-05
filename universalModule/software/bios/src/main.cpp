#include <cassert>
#include <system/dbg.hpp>
#include <drivers/gpio.hpp>
#include <drivers/uart.hpp>
#include <drivers/clock.hpp>
#include <drivers/dma.hpp>

#include <stm32f0xx_hal.h>
#include <stm32f0xx_ll_gpio.h>

Dbg& dbgInstance() {
    static Dbg inst(
        USART1, LL_DMA_CHANNEL_3, LL_DMA_CHANNEL_2,
        TxOn( GpioB[ 6 ] ),
        RxOn( GpioB[ 7 ] ),
        Baudrate( 115200 ) );
    return inst;
}

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();
    HAL_Init();

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_6);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_7);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    while ( true ) {
        LL_GPIO_SetOutputPin( GPIOB, LL_GPIO_PIN_6 );
        LL_GPIO_ResetOutputPin( GPIOB, LL_GPIO_PIN_6 );
        LL_GPIO_SetOutputPin( GPIOB, LL_GPIO_PIN_7 );
        LL_GPIO_ResetOutputPin( GPIOB, LL_GPIO_PIN_7 );
    }

    auto pin = GpioB[ 6 ];
    pin.setupPPOutput();
    while ( true ) {
        pin.write( true );
        HAL_Delay( 1 );
        pin.write( false );
        HAL_Delay( 1 );
    }

    // Dbg::error( "Main clock: %d", SystemCoreClock );
    // HAL_Delay( 100 );

    // while ( true ) {
    //     Dbg::error( "Hello world!" );
    // }
}

