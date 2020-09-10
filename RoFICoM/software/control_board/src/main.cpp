
#include <system/defer.hpp>

#include <cassert>
#include <system/dbg.hpp>
#include <drivers/clock.hpp>
#include <drivers/gpio.hpp>
#include <drivers/timer.hpp>
#include <drivers/spi.hpp>
#include <drivers/uart.hpp>
#include <drivers/adc.hpp>

#include <motor.hpp>
#include <util.hpp>

#include <spiInterface.hpp>
#include <connInterface.hpp>

#include <stm32g0xx_hal.h>


using Block = memory::Pool::Block;

Dbg& dbgInstance() {
    static Dbg inst(
        USART1, LL_DMA_CHANNEL_1, LL_DMA_CHANNEL_2,
        TxOn( GpioB[ 6 ] ),
        RxOn( GpioB[ 7 ] ),
        Baudrate( 115200 ) );
    return inst;
}

void onCmdVersion( SpiInterface& interf ) {
    auto block = memory::Pool::allocate( 4 );
    viewAs< uint16_t >( block.get() ) = 1;
    viewAs< uint16_t >( block.get() + 2 ) = 1;
    interf.sendBlock( std::move( block ), 4 );
}

void onCmdStatus( SpiInterface& interf, Block /*header*/, ConnComInterface& connInt ) {
    auto block = memory::Pool::allocate( 12 );
    memset( block.get(), 0xAA, 12 );
    viewAs< uint8_t >( block.get() + 2 ) = connInt.pending();
    viewAs< uint8_t >( block.get() + 3 ) = connInt.available();
    // ToDo: Assign remaining values
    interf.sendBlock( std::move( block ), 12 );
    // ToDo Interpret the header
}

void onCmdInterrupt( SpiInterface& interf, Block /*header*/ ) {
    auto block = memory::Pool::allocate( 2 );
    viewAs< uint16_t >( block.get() ) = 0;
    interf.sendBlock( std::move( block ), 2 );
}

void onCmdSendBlob( SpiInterface& spiInt, ConnComInterface& connInt ) {
    spiInt.receiveBlob([&]( Block blob, int size ) {
        int blobLen = viewAs< uint16_t >( blob.get() + 2 );
        if ( size != 4 + blobLen )
            return;
        Dbg::info( "CMD blob send received: %d, %.*s", blobLen, blobLen, blob.get() + 4 );
        connInt.sendBlob( std::move( blob ) );
    } );
}

void onCmdReceiveBlob( SpiInterface& spiInt, ConnComInterface& connInt ) {
    if ( connInt.available() > 0 ) {
        Block blob = connInt.getBlob();
        spiInt.sendBlob( std::move( blob ) );
    }
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

    Dbg::error( "Main clock: %d", SystemCoreClock );

    Adc1.setup();
    Adc1.enable();

    Timer timer( TIM1, FreqAndRes( 1000, 2000 ) );
    auto pwm = timer.pwmChannel( LL_TIM_CHANNEL_CH1 );
    pwm.attachPin( GpioA[ 8 ] );
    timer.enable();

    Motor motor( pwm, GpioB[ 1 ] );
    motor.enable();
    motor.set( 0 );

    Slider slider( Motor( pwm, GpioB[ 1 ] ), GpioA[ 7 ], GpioA[ 5 ] );

    PowerSwitch powerInterface;
    ConnectorStatus connectorStatus ( GpioC[ 6 ], GpioA[ 15 ] );


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

    ConnComInterface connComInterface( std::move( uart ) );

    using Command = SpiInterface::Command;
    SpiInterface spiInterface( std::move( spi ), GpioA[ 4 ],
        [&]( Command cmd, Block b ) {
            switch( cmd ) {
            case Command::VERSION:
                onCmdVersion( spiInterface );
                break;
            case Command::STATUS:
                onCmdStatus( spiInterface, std::move( b ), connComInterface );
                break;
            case Command::INTERRUPT:
                onCmdInterrupt( spiInterface, std::move( b ) );
                break;
            case Command::SEND_BLOB:
                onCmdSendBlob( spiInterface, connComInterface );
                break;
            case Command::RECEIVE_BLOB:
                onCmdReceiveBlob( spiInterface, connComInterface );
                break;
            default:
                Dbg::warning( "Unknown command %d", cmd );
            };
        } );
    connComInterface.onNewBlob( [&] { spiInterface.interruptMaster(); } );

    while ( true ) {
        slider.run();
        powerInterface.run();
        if ( connectorStatus.run() )
            spiInterface.interruptMaster();

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
                Dbg::error( "DBG received: %c", Dbg::get() );
            }
        }
        Defer::run();
    }
}

