#pragma once

#include <drivers/uart.hpp>
#include <drivers/crc.hpp>
#include <drivers/gpio.hpp>
#include <system/defer.hpp>
#include <system/ringBuffer.hpp>
#include <blob.hpp>

class ConnComInterface {
public:
    static const int BLOB_LIMIT = 512;
    using Block = memory::Pool::Block;

    ConnComInterface( Uart uart )
        : _uart( std::move( uart ) ),
          _reader( _uart, Dma::allocate( DMA1 ) ),
          _writer( _uart, Dma::allocate( DMA1 ) ),
          _busy( false ),
          _inQueue( memory::Pool::allocate( 64 ), 64 ),
          _outQueue( memory::Pool::allocate( 64 ), 64 )
    {
        _uart.enable();
        _receiveFrame();
    }

    int available() const { return _inQueue.size(); }
    int pending() const { return _outQueue.size(); }

    Block getBlob() {
        assert( _inQueue.size() > 0 );
        return _inQueue.pop_front();
    }

    void sendBlob( Block blob ) {
        assert( blob.get() );
        Defer::job([&, b = std::move( blob )]() mutable {
            HAL_Delay(100);
            int length = blobLen( b );
            uint32_t crc = Crc::compute( blobBody( b ), length );
            crcField( b ) = crc;
            _outQueue.push_back( std::move( b ) );
            if ( !_busy )
                _transmitFrame( _outQueue.pop_front() );
        });
    }

    template < typename Callback >
    void onNewBlob( Callback c ) {
        _notifyNewBlob = c;
    }
private:
    void _receiveFrame() {
        _reader.readBlock( memory::Pool::allocate( 1 ), 0, 1, 0,
            [&]( Block b, int size ) {
                if ( size == 0 || b[ 0 ] != 0xAA ) {
                    _receiveFrame();
                    return;
                }
                _reader.readBlock( memory::Pool::allocate( 1 ), 0, 1, _timeout,
                    [&]( Block /**/, int size ) {
                        if ( size == 0 ) {
                            Dbg::error("I2");
                            _receiveFrame();
                            return;
                        }
                        _receiveBlob();
                    } );
            } );
    }

    void _receiveBlob() {
        auto buffer = memory::Pool::allocate( BLOB_LIMIT );
        if ( !buffer ) {
            Dbg::warning( "Cannot allocate memory for reception" );
            _receiveFrame();
            return;
        }
        _reader.readBlock( std::move( buffer ), 0, BLOB_HEADER_SIZE, _timeout,
            [&]( Block b, int size ) {
                if ( size != BLOB_HEADER_SIZE ) {
                    Dbg::error("I3, %d", size);
                    _receiveFrame();
                    return;
                }
                uint16_t length = blobLen( b );
                if ( length > BLOB_LIMIT ) {
                    Dbg::error("I4");
                    _receiveFrame();
                    return;
                }
                _reader.readBlock( std::move( b ), BLOB_HEADER_SIZE, length + CRC_SIZE, _timeout,
                    [&]( Block b, int s ) {
                        Dbg::error("I5");
                        _onNewBlob( std::move( b ), s );
                        _receiveFrame();
                    } );
            } );
    }

    void _onNewBlob( Block b, int size ) {
        Defer::job([&, blob = std::move( b ), size ]() mutable {
            int length = blobLen( blob );
            if ( length + CRC_SIZE != size ) {
                Dbg::warning( "Invalid blob size, %d", length );
                return;
            }
            uint32_t crc = Crc::compute( blobBody( blob ), length );
            if ( crc != crcField( blob ) ) {
                Dbg::warning( "Blob CRC mismatch" );
                return;
            }
            Dbg::info( "New blob received: %d, %.*s", length, length, blob.get() + 4 );
            if ( !_inQueue.push_back( std::move( blob ) ) ) {
                Dbg::info( "Queue is full" );
            }
            if ( _notifyNewBlob )
                _notifyNewBlob();
        } );
    }

    void _transmitFrame( Block blob ) {
        Dbg::error("Starting to trasmit!");
        while ( _busy );
        _busy = true;
        Dbg::error("Ready to transmit!");

        _txBlock = std::move( blob );
        Block header = memory::Pool::allocate(2);
        header[0] = 0xAA;
        header[1] = 0;
        _writer.writeBlock( std::move( header ), 0, 2,
            [&]( Block /*header*/, int size ) {
                if ( size != 2 ) {
                    Dbg::error("Failed with %d", size);
                    _busy = false;
                    return;
                }
                _transmitBlob( std::move( _txBlock ) );
            } );
    }

    void _transmitBlob( Block blob ) {
        uint16_t length = blobLen( blob );
        _writer.writeBlock( std::move( blob ), 0, 4 + length + CRC_SIZE,
            [&]( Block /*blob*/, int /*size*/) {
                _busy = false;
                if ( !_outQueue.empty() )
                    _transmitFrame( _outQueue.pop_front() );
            } );
    }

    Uart _uart;
    UartReader< memory::Pool > _reader;
    UartWriter< memory::Pool > _writer;
    volatile bool _busy;
    RingBuffer< Block, memory::Pool > _inQueue, _outQueue;
    std::function< void(void) > _notifyNewBlob;
    Block _txBlock;
    static const int _timeout = 64;
};

enum ConnectorOrientation {
    North = 0, South = 1, East = 2, West = 3, Unknown = 4
};

class ConnectorStatus {
public:
    ConnectorStatus( Gpio::Pin senseA, Gpio::Pin senseB )
        : _senseA( senseA ), _senseB( senseB ), _orientation( Unknown )
    {
        _senseA.setupInput( true );
        _senseB.setupInput( true );
    }

    /**
     * \brief Run internal routine of getting connector status
     *
     * \return true, if state was changed (and the master is expected to be
     * interrupted)
     */
    bool run() {
        auto aState = pinState( _senseA );
        if ( aState == PinState::High )
            return onNewState( North );
        if ( aState == PinState::Low )
            return onNewState( West );
        auto bState = pinState( _senseB );
        if ( bState == PinState::High )
            return onNewState( South );
        if ( bState == PinState::Low )
            return onNewState( East );
        return onNewState( Unknown );
    }

    ConnectorOrientation getOrientation() {
        return _orientation;
    }
private:
    enum class PinState { Float, High, Low };

    /**
     * \brief Check state of given pin
     *
     * Internally checks if pin value matches pull-up/down resistor value. If
     * so, the pin floats, otherwise it is in given state.
     */
    static PinState pinState( Gpio::Pin& pin ) {
        pin.setupInput( true, true ); // Pull-up
        if ( !pin.read() )
            return PinState::Low;
        pin.setupInput( true, false ); // Pull-down
        if ( pin.read() )
            return PinState::High;
        return PinState::Float;
    }

    bool onNewState( ConnectorOrientation orientation ) {
        bool ret = orientation != _orientation;
        _orientation = orientation;
        return ret;
    }

    Gpio::Pin _senseA, _senseB;
    ConnectorOrientation _orientation;
};

class PowerSwitch {
public:
    void run() {
        // ToDo
    }

    /**
     * \brief Get voltage on the internal bus
     *
     * \return Fixed point <8.8> number in volts
     */
    uint16_t getIntVoltage() {
        // ToDo
        return 0;
    };

    /**
     * \brief Get voltage on the external bus
     *
     * \return Fixed point <8.8> number in volts
     */
    uint16_t getExtVoltage() {
        // ToDo
        return 0;
    };

    /**
     * \brief Get current on the internal bus
     *
     * \return Fixed point <8.8> number in amperes
     */
    uint16_t getIntCurrent() {
        // ToDo
        return 0;
    };

    /**
     * \brief Get current on the external bus
     *
     * \return Fixed point <8.8> number in amperes
     */
    uint16_t getExtCurrent() {
        // ToDo
        return 0;
    };

    void connectInternal( bool /*connect = true*/ ) {
        // ToDo
    }

    void connectExternal( bool /*connect = true*/ ) {
        // ToDo
    }
private:
    uint16_t _intVoltage, intCurrent;
    uint16_t _extVoltage, extCurrent;
};