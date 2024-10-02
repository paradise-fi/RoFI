#include <cassert>
#include <system/dbg.hpp>
#include <drivers/gpio.hpp>
#include <drivers/uart.hpp>
#include <drivers/clock.hpp>
#include <drivers/dma.hpp>
#include <drivers/adc.hpp>

#include <stm32f0xx_hal.h>
#include <stm32f0xx_ll_gpio.h>

#include "phy.hpp"

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

    Phy& phy = Phy::inst();
    LineReader< Dbg > reader( dbgInstance() );

    phy.setup();
    phy.enableUsbBridge( true );
    phy.enableUsbToBusConverter();
    phy.enableBattToBusConverter( false );
    phy.enableCharger( false );

    while ( true ) {
        HAL_Delay( 100 );
        // Dbg::error( "%d, %d, %d", phy.btnLeft.read(), phy.btnMid.read(),
        //     phy.btnRight.read() );
        Dbg::info( "batt: %f, bus: %f, usb: %f",
            phy.batteryVoltage(), phy.busVoltage(), phy.usbVoltage() );
        if ( phy.btnLeft.read() )
            phy.enableCharger();
        else
            phy.enableCharger( false );

        if ( reader.available() ) {
            Dbg::error( "Available: %s", reader.get().get() );
        }
    }
}
