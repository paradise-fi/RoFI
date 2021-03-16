#pragma once

#include <drivers/uart.hpp>
#include <system/memory.hpp>
#include <printf.h>

class Dbg;
extern Dbg& dbgInstance();

class Dbg {
    template < typename... Configs >
    Dbg( USART_TypeDef *uartPer, int dmaRead, int dmaWrite, Configs...configs ):
        _uart( uartPer, configs... ), _writer( _uart, dmaWrite ), _reader( _uart, dmaRead )
    {
        _uart.enable();
        _reader.startBufferedReading();
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

    template < typename T >
    static void dumpReg( const char *name, T val ) {
        char buff[ 8 * sizeof( T ) + 1 ];
        for( int i = 0; i != 8 * sizeof( T ); i++ ) {
            buff[ i ] = ( val & ( 1 << ( 8 * sizeof( T ) - 1 - i) ) )
                ? '1'
                : '0';
        }
        buff[ 8 * sizeof( T ) ] = 0;
        info( "%s: %s", name, buff );
    }

    static bool available() {
        return _instance()._reader.available();
    }

    static char get() {
        return _instance()._reader.get();
    }

private:
    friend Dbg& dbgInstance();

    static Dbg& _instance() {
        return dbgInstance();
    }

    template < typename... Args >
    void _info( const char *fmt, Args...args ) {
        while ( _txBusy );
        _txBusy = true;

        auto buffer = memory::Pool::allocate( 256 );
        assert( buffer );
        int size = snprintf( reinterpret_cast< char * >( buffer.get() ), 256, fmt, args... );
        buffer[ size ] = '\n';
        _writer.writeBlock( std::move( buffer ), 0, size + 1, [&]( memory::Pool::Block, int ){
            _txBusy = false;
        } );
    }

    static char *errorBuffer() {
        static char buffer[ 512 ];
        return buffer;
    }

    template < typename... Args >
    void _error( const char *fmt, Args...args ) {
        _writer.abort();
        _txBusy = false;

        char *buffer = errorBuffer();
        int size = snprintf( buffer, 510, fmt, args... );
        buffer[ size ] = '\n';
        buffer[ size + 1 ] = 0;
        _uart.send( buffer );
    }

    template < typename... Args >
    void _warning( const char *fmt, Args...args ) {
        _info( fmt, args... );
    }

    Uart _uart;
    UartWriter _writer;
    UartReader _reader;
    volatile bool _txBusy;
};
