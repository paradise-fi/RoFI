#pragma once

#include <drivers/uart.hpp>
#include <drivers/crc.hpp>
#include <drivers/gpio.hpp>
#include <system/idle.hpp>
#include <system/ringBuffer.hpp>
#include <system/irq.hpp>
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
        IdleTask::defer([this, b = std::move( blob )]() mutable {
            int length = blobLen( b );
            uint32_t crcRh = Crc::compute( blobBody( b ), length );
            Dbg::blockingInfo("Sending: ");
            Dbg::hexDump( blobBody( b ), length );
            crcField( b ) = crcRh;
            _outQueue.push_back( std::move( b ) );
        });
    }

    template < typename Callback >
    void onNewBlob( Callback c ) {
        _notifyNewBlob = c;
    }

    void run() {
        if ( !_busy && !_outQueue.empty() )
            _transmitFrame( _outQueue.pop_front() );
    }
private:
    void _receiveFrame() {
        auto buffer = memory::Pool::allocate( BLOB_LIMIT );
        if ( !buffer.get() ) {
            Dbg::warning( "Cannot allocate memory for reception" );
            _receiveFrame();
            return;
        }

        _reader.readBlock( std::move( buffer ), 0, 2, 0,
            [ this ] ( Block rxBuffer, int size ) {
                if ( size != 2 || rxBuffer[ 0 ] != 0xAA ) {
                    _receiveFrame();
                    return;
                }
                _receiveBlob( std::move( rxBuffer ) );
            } );
    }

    void _receiveBlob( memory::Pool::Block buffer ) {
        _reader.readBlock( std::move( buffer ), 0, BLOB_HEADER_SIZE, _timeout,
            [&]( Block b, int size ) {
                if ( size != BLOB_HEADER_SIZE ) {
                    Dbg::blockingInfo("E2 %d", size);
                    _receiveFrame();
                    return;
                }
                uint16_t length = blobLen( b );
                if ( length > BLOB_LIMIT ) {
                    Dbg::blockingInfo("E3");
                    _receiveFrame();
                    return;
                }
                _reader.readBlock( std::move( b ), BLOB_HEADER_SIZE, length + CRC_SIZE, _timeout,
                    [&]( Block b, int s ) {
                        _onNewBlob( std::move( b ), s );
                        _receiveFrame();
                    } );
            } );
    }

    void _onNewBlob( Block b, int size ) {
        IdleTask::defer([this, blob = std::move( b ), size ]() mutable {
            Dbg::blockingInfo( "Receiving" );
            Dbg::hexDump( blob.get(), size );
            int length = blobLen( blob );
            if ( length + CRC_SIZE != size ) {
                Dbg::warning( "Invalid blob size, %d, %d, %d", length, size - CRC_SIZE, size );
                return;
            }
            uint32_t crc = Crc::compute( blobBody( blob ), length );
            if ( crc != crcField( blob ) ) {
                Dbg::warning( "Blob CRC mismatch, %lu, %lu", crc, crcField( blob ) );
                return;
            }
            if ( !_inQueue.push_back( std::move( blob ) ) ) {
                Dbg::info( "Queue is full" );
            }
            if ( _notifyNewBlob )
                _notifyNewBlob();
        } );
    }

    void _transmitFrame( Block blob ) {
        while ( _busy );
        _busy = true;

        _txBlock = std::move( blob );
        Block header = memory::Pool::allocate(2);
        header[0] = 0xAA;
        header[1] = 0;
        _writer.writeBlock( std::move( header ), 0, 2,
            [this]( Block /*header*/, int size ) {
                if ( size != 2 ) {
                    Dbg::warning( "Failed with %d", size );
                    _busy = false;
                    return;
                }
                _transmitBlob( std::move( _txBlock ) );
            } );
    }

    void _transmitBlob( Block blob ) {
        uint16_t length = blobLen( blob );
        Dbg::blockingInfo("TLen: %d", length);
        _writer.writeBlock( std::move( blob ), 0, 4 + length + CRC_SIZE,
            [this]( Block /*blob*/, int /*size*/) {
                _busy = false;
            } );
    }

    Uart _uart;
    UartReader< memory::Pool > _reader;
    UartWriter< memory::Pool > _writer;
    volatile bool _busy;
    RingBuffer< Block, memory::Pool > _inQueue, _outQueue;
    std::function< void(void) > _notifyNewBlob;
    Block _txBlock;
    static const int _timeout = 2048;
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
    PowerSwitch(
        Gpio::Pin intSwitch
        , Gpio::Pin intVoltage
        , Gpio::Pin intCurrent
        , Gpio::Pin extSwitch
        , Gpio::Pin extVoltage
        , Gpio::Pin extCurrent
    )
    : _intSwitchPin( intSwitch )
    , _intVoltagePin( intVoltage )
    , _intCurrentPin( intCurrent )
    , _extSwitchPin( extSwitch )
    , _extVoltagePin( extVoltage )
    , _extCurrentPin( extCurrent )
    {
        _intVoltagePin.setupAnalog();
        _extVoltagePin.setupAnalog();
        _intCurrentPin.setupAnalog();
        _extCurrentPin.setupAnalog();
        _intSwitchPin.setupODOutput( false );
        _extSwitchPin.setupODOutput( false );
    }

    void run() {
        _intVoltage = pinToResult( _intVoltagePin ) * 255.0;
        _extVoltage = pinToResult( _extVoltagePin ) * 255.0;
        // ToDo ext and int current which is currently not present on the board
    }

    /**
     * \brief Get voltage on the internal bus
     *
     * \return Fixed point <8.8> number in volts
     */
    uint16_t getIntVoltage() {
        return _intVoltage;
    };

    /**
     * \brief Get voltage on the external bus
     *
     * \return Fixed point <8.8> number in volts
     */
    uint16_t getExtVoltage() {
        return _extVoltage;
    };

    /**
     * \brief Get current on the internal bus
     *
     * \return Fixed point <8.8> number in amperes
     */
    uint16_t getIntCurrent() {
        return _intCurrent;
    };

    /**
     * \brief Get current on the external bus
     *
     * \return Fixed point <8.8> number in amperes
     */
    uint16_t getExtCurrent() {
        return _extCurrent;
    };

    bool getInternalConnection() {
        return _intConnected;
    }

    bool getExternalConnection() {
        return _extConnected;
    }

    void connectInternal( bool connect = true ) {
        _intConnected = connect;
        _intSwitchPin.write( connect );
    }

    void connectExternal( bool connect = true ) {
        _extConnected = connect;
        _extSwitchPin.write( connect );
    }

private:
    float pinToResult( Gpio::Pin pin ) {
        return float( pin.readAnalog() ) * (100.0 + 6.8) * 3.3 / ( 6.8 * 4096 );
    }

    uint16_t _intVoltage = 0, _intCurrent = 0;
    uint16_t _extVoltage = 0, _extCurrent = 0;

    Gpio::Pin _intSwitchPin;
    Gpio::Pin _intVoltagePin;
    Gpio::Pin _intCurrentPin;
    Gpio::Pin _extSwitchPin;
    Gpio::Pin _extVoltagePin;
    Gpio::Pin _extCurrentPin;

    bool _intConnected = false, _extConnected = false;

};
