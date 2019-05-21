
#include <system/defer.hpp>

#include <cassert>
#include <system/dbg.hpp>
#include <system/clock.hpp>
#include <drivers/gpio.hpp>
#include <drivers/timer.hpp>
#include <drivers/spi.hpp>
#include <drivers/uart.hpp>
#include <motor.hpp>
#include <util.hpp>

#include <spiInterface.hpp>
#include <connInterface.hpp>

#include <stm32g0xx_hal.h>


using Block = memory::Pool::Block;

void onCmdVersion( SpiInterface& interf ) {
    auto block = memory::Pool::allocate( 4 );
    viewAs< uint16_t >( block.get() ) = 1;
    viewAs< uint16_t >( block.get() + 2 ) = 0;
    interf.sendBlock( std::move( block ), 4 );
}

void onCmdStatus( SpiInterface& interf, Block /*header*/, ConnInterface& connInt ) {
    auto block = memory::Pool::allocate( 12 );
    memset( block.get(), 0xAA, 12 );
    viewAs< uint8_t >( block.get() + 2 ) = connInt.pending();
    viewAs< uint8_t >( block.get() + 3 ) = connInt.available();
    // ToDo: Assign remaining values
    interf.sendBlock( std::move( block ), 12 );
    // ToDo Interpret the header
}

void onCmdInterrupt( SpiInterface& interf, Block /*header*/ ) {
    Defer::job( []{
        Dbg::info( "Interrupt command" );
    } );
    auto block = memory::Pool::allocate( 2 );
    memset( block.get(), 0, 2 );
    interf.sendBlock( std::move( block ), 2 );
}

void onCmdSendBlob( SpiInterface& spiInt, ConnInterface& connInt ) {
    spiInt.receiveBlob([&]( Block blob, int size ) {
        uint16_t blobLen = viewAs< uint16_t >( blob.get() + 2 );
        if ( size != 4 + blobLen )
            return;
        connInt.sendBlob( std::move( blob ) );
    } );
}

void onCmdReceiveBlob( SpiInterface& spiInt, ConnInterface& connInt ) {
    if ( connInt.available() > 0 )
        spiInt.sendBlob( connInt.getBlob() );
    else {
        auto block = memory::Pool::allocate( 4 );
        viewAs< uint16_t >( block.get() ) = 0; // content type
        viewAs< uint16_t >( block.get() + 2 ) = 0; // length
        spiInt.sendBlock( std::move( block ), 4 );
    }
}

int main() {
    setupSystemClock();
    SystemCoreClockUpdate();
    HAL_Init();

    Dbg::info( "Main clock: %d", SystemCoreClock );

    Timer timer( TIM1, FreqAndRes( 1000, 2000 ) );
    auto pwm = timer.pwmChannel( LL_TIM_CHANNEL_CH1 );
    pwm.attachPin( GpioA[ 8 ] );
    timer.enable();

    Motor motor( pwm, GpioB[ 1 ] );
    motor.enable();
    motor.set( 0 );

    Slider slider( Motor( pwm, GpioB[ 1 ] ), GpioA[ 7 ], GpioA[ 5 ] );

    Spi spi( SPI1,
        Slave(),
        MisoOn( GpioA[ 6 ] ),
        SckOn( GpioA[ 1 ] ),
        CsOn( GpioA[ 4 ] )
    );

    Uart uart( USART2,
        Baudrate( 115200 ),
        TxOn( GpioA[ 2 ] ),
        RxOn( GpioA[ 3 ] ) );

    ConnInterface connInterface( std::move( uart ) );

    using Command = SpiInterface::Command;
    SpiInterface spiInterface( std::move( spi ),
        [&]( Command cmd, Block b ) {
            // Dbg::info( "Command %d", cmd );
            switch( cmd ) {
            case Command::VERSION:
                onCmdVersion( spiInterface );
                break;
            case Command::STATUS:
                onCmdStatus( spiInterface, std::move( b ), connInterface );
                break;
            case Command::INTERRUPT:
                onCmdInterrupt( spiInterface, std::move( b ) );
                break;
            case Command::SEND_BLOB:
                onCmdSendBlob( spiInterface, connInterface );
                break;
            case Command::RECEIVE_BLOB:
                onCmdReceiveBlob( spiInterface, connInterface );
                break;
            default:
                Dbg::warning( "Unkwnown command %d", cmd );
            };
        } );

    while ( true ) {
        slider.run();
        if ( Dbg::available() ) {
            switch( Dbg::get() ) {
            case 'e':
                slider.expand();
                Dbg::info("Expanding");
                break;
            case 'r':
                slider.retract();
                Dbg::info("Retracting");
                break;
            default:
                Dbg::info( "Received: %c", Dbg::get() );
            }
        }
        Defer::run();
    }
}

