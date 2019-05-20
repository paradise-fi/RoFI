#pragma once

#include <functional>
#include <drivers/spi.hpp>
#include <util.hpp>

class SpiInterface {
public:
    enum Command { VERSION = 0, STATUS, INTERRUPT, SEND_BLOB, RECEIVE_BLOB };
    static const int BLOB_LIMIT = 512;
    using Block = memory::Pool::Block;
    using CmdHandler = std::function< void( Command, Block ) >;

    SpiInterface( Spi spi, CmdHandler cmdHandler )
        : _spi( std::move( spi ) ), _spiRw( _spi ), _cmdHandler( std::move( cmdHandler ) )
    {
        _spi.onTransactionEnds( [&]{ _startCommand(); } );
        _spi.enable();
        _startCommand();
    }

    template < typename Callback >
    void receiveBlob( Callback c ) {
        auto buffer = memory::Pool::allocate( BLOB_LIMIT );
        if ( !buffer ) {
            Dbg::warning( "Cannot allocate memory for reception" );
            return;
        }
        _spiRw.readBlock( std::move( buffer ), 0, 4,
            [&, c]( Block b, int size ) {
                if ( size != 4 )
                    return;
                uint16_t length = viewAs< uint16_t >( b.get() + 2 );
                _spiRw.readBlock( std::move( b ), 4, length,
                    [&, length, c]( Block b, int s ) {
                        c( std::move( b ), 4 + s );
                    } );
            } );
    }

    void sendBlob( Block blob ) {
        uint16_t length = viewAs< uint16_t >( blob.get() + 2 );
        _spiRw.writeBlock( std::move( blob ), 0, 4 + length,
            []( Block /*block*/, int /*size*/){} );
    }

    void sendBlock( Block block, int size, int offset = 0 ) {
        _spiRw.writeBlock( std::move( block ), offset, size,
            []( Block /*block*/, int /*size*/){} );
    }

    void interruptMaster() {
        // ToDo
    }

private:
    void _startCommand() {
        _abortCommand();
        _spi.clearRx();
        _spiRw.readBlock( memory::Pool::allocate( 1 ), 0, 1,
            [&]( Block b, int s ) {
                if ( s == 0 )
                    return;
                auto command = static_cast< Command >( b[ 0 ] );
                int size = _commandLen( command );
                if ( size == 0  ) {
                    _cmdHandler( command, nullptr );
                    return;
                }
                _spiRw.readBlock( memory::Pool::allocate( size ), 0, size,
                    [&, command ]( Block b, int s ) {
                        if ( s != _commandLen( command ) )
                            return;
                        _cmdHandler( command, std::move( b ) );
                    } );
            } );
    }

    static int _commandLen( Command c ) {
        switch( c ) {
            case Command::VERSION: return 0;
            case Command::STATUS: return 4;
            case Command::INTERRUPT: return 2;
            case Command::SEND_BLOB: return 0;
            case Command::RECEIVE_BLOB: return 0;
            default: return 0;
        }
    }

    void _abortCommand() {
        _spi.disableRx();
        _spi.disableTx();
        _spiRw.abortRx();
        _spiRw.abortTx();
    }

    Spi _spi;
    SpiReaderWriter _spiRw;
    CmdHandler _cmdHandler;
};