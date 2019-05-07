#pragma once

#include <drivers/uart.hpp>
#include <system/memory.hpp>

class Dbg {
    template < typename... Configs >
    Dbg( USART_TypeDef *uartPer, Configs...configs ):
        _uart( uartPer, configs... ), writer( _uart )
    {
        _uart.enable();
    }
public:
    template < typename... Args >
    static void info( const char *fmt, Args...args ) {
        _instance()._info( fmt, args... );
    }

    template < typename... Args >
    static void error( const char *fmt, Args...args ) {
        _instance()._error( fmt, args... );
    }

    template < typename... Args >
    static void warning( const char *fmt, Args...args ) {
        _instance()._warning( fmt, args... );
    }

private:
    static Dbg& _instance() {
        static Dbg inst( USART1,
            TxOn( GpioB[ 6 ] ),
            RxOn( GpioB[ 7 ] ),
            Baudrate( 115200 ) );
        return inst;
    }

    template < typename... Args >
    void _info( const char *fmt, Args...args ) {
        while ( _txBusy );
        _txBusy = true;

        auto buffer = memory::Pool::allocate( 256 );
        int size = snprintf( reinterpret_cast< char * >( buffer.get() ), 256, fmt, args... );
        buffer[ size ] = '\n';
        writer.writeBlock( std::move( buffer ), 0, size + 1, [&]( memory::Pool::Block ){
            _txBusy = false;
        } );
    }

    template < typename... Args >
    void _error( const char *fmt, Args...args ) {
        _info( fmt, args... );
    }

    template < typename... Args >
    void _warning( const char *fmt, Args...args ) {
        _info( fmt, args... );
    }

    Uart _uart;
    UartWriter writer;
    volatile bool _txBusy;
};