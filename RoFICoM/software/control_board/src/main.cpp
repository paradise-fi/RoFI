#include <stm32g0xx_hal.h>

#include <cassert>
#include <system/dbg.hpp>
#include <system/clock.hpp>
#include <drivers/gpio.hpp>
#include <drivers/timer.hpp>
#include <drivers/spi.hpp>
#include <motor.hpp>

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();
    HAL_Init();

    Dbg::info( "Main clock: %d", SystemCoreClock );
    int counter = 0;
    GpioA[ 7 ].setupInput( true );
    GpioB[ 1 ].setupPPOutput();

    Timer timer( TIM1, FreqAndRes( 1000, 2000 ) );
    auto pwm = timer.pwmChannel( LL_TIM_CHANNEL_CH1 );
    pwm.attachPin( GpioA[ 8 ] );
    timer.enable();

    Motor motor( pwm, GpioB[ 1 ] );
    motor.enable();
    motor.set( 0 );

    Spi spi( SPI1,
        Slave(),
        MisoOn( GpioA[ 6 ] ),
        SckOn( GpioA[ 1 ] ),
        CsOn( GpioA[ 4 ] )
    );
    SpiReaderWriter rwSpi( spi );

    volatile bool print = false;
    memory::Pool::Block rxBuf;
    auto txBuf = memory::Pool::allocate( 40 );
    std::copy_n( "Hello world", 20, txBuf.get() );
    int tId = 0;
    spi.onTransactionEnds( [&] {
        tId++;
        Dbg::info( "Transcation prepare %d", tId );
        rwSpi.abortRx();
        rwSpi.abortTx();
        spi.clearRx();
        rwSpi.readBlock( memory::Pool::allocate( 11 ), 0, 10,
            [&, tId]( memory::Pool::Block b, int size ) {
                rxBuf = std::move( b );
                rxBuf [ size ] = 0;
                print = true;
                rwSpi.writeBlock( std::move( txBuf ), 0, 10,
                    [&, tId]( memory::Pool::Block b, int /* size */ ) {
                        txBuf = std::move( b );
                    } );
            } );
    } );

    spi.enable();

    while ( true ) {
        if ( print ) {
            counter++;
            print = false;
            Dbg::info( "Received (%d): %10.10s", counter, rxBuf.get() );
        }
    }
}

