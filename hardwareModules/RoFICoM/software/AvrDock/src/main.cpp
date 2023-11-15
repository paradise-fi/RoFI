#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "util.hpp"
#include "blob.hpp"
#include "modernHack.hpp"

class D2DInterface;
template < typename T >
D2DInterface& _print( T t );

void onAllocFail();

Blob allocate( uint8_t size ) {
    for ( int i = 0; i != 4; i++ ) {
        Blob b = Blob::allocate( size );
        if ( !b )
            onAllocFail();
        else
            return b;
    }
    return Blob();
}

enum SpiCommand { VERSION = 0, STATUS, INTERRUPT, SEND_BLOB, RECEIVE_BLOB };

template < typename T >
T& as( uint8_t* p ) {
    return *reinterpret_cast< T * >( p );
}

class SpiInterface {
private:
    enum class State { HEADER, RECEIVE, SEND };
public:
    SpiInterface(): _interrupt( false ) {
        _makeMisoOut();
        SPCR = Bitmask8( SPE, SPIE );
    }

    bool headerReceived() {
        if ( _progress == 0 )
            return false;
        return _state == State::HEADER && _progress == headerSize( _header[ 0 ] );
    }

    uint8_t* header() { return _header; }

    bool blobReceived() {
        bool b = _received;
        _received = false;
        return b;
    }

    Blob getBlob() {
        return move( _buffer );
    }

    void receive( uint8_t offset = 0 ) {
        nextState( State::RECEIVE );
        _progress = offset;
        _buffer = move( allocate( MAX_BUFFER_SIZE ) );
    }

    void send( Blob&& b, uint8_t limit, uint8_t offset = 0 ) {
        nextState( State::SEND );
        _limit = limit;
        _progress = offset;
        _buffer = move( b );
        shiftOutBuffer();
    }

    void onSpiEnd() {
        SPDR = 0;
        nextState( State::HEADER );
        _progress = 0;
        if ( _prevState == State::SEND ) // Optimization
            _buffer.free();
        if ( _interrupt ) {
            InterruptGuard _;
            triggerMasterInterrutp();
            _interrupt = false;
        }
    }

    void onByte() {
        if ( _state == State::SEND ) {
            if ( _buffer )
                shiftOutBuffer();
        } else {
            SPDR = 0;
            if ( _state == State::HEADER && _progress < 5 ) {
                _header[ _progress++ ] = SPDR;
            }
            else {
                if ( _buffer && _progress < MAX_BUFFER_SIZE )
                    _buffer[ _progress++ ] = SPDR;
            }
        }
    }

    void interruptMaster() {
        InterruptGuard _;
        if ( _getSS() )
            triggerMasterInterrutp();
        else
            _interrupt = true;
    }


    void shiftOutBuffer() {
        if ( _progress < _limit )
            SPDR = _buffer[ _progress++ ];
    }

    void nextState( State s ) {
        _prevState = _state;
        _state = s;
        _received = _prevState == State::RECEIVE && _state == State::HEADER;
    }

    void triggerMasterInterrutp() {
        uint8_t state = SPCR;
        SPCR = 0;
        _makeSSOut();
        _resetSS();
        _setSS();
        _makeSSIn();
        SPCR = state;
    }

    static uint8_t headerSize( uint8_t command ) {
        switch ( command ) {
            case SpiCommand::VERSION: return 1;
            case SpiCommand::STATUS: return 5;
            case SpiCommand::INTERRUPT: return 3;
            case SpiCommand::SEND_BLOB: return 1;
            case SpiCommand::RECEIVE_BLOB: return 1;
            default: return 1;
        }
    }

    void _makeMisoOut() {  DDRB |= Bitmask8( PB4 ); }
    void _makeSSOut() { DDRB |= Bitmask8( PB2 ); }
    void _makeSSIn() { DDRB &= ~Bitmask8( PB2 ); }
    void _setSS() { PORTB |= Bitmask8( PB2 ); }
    void _resetSS() { PORTB &= ~Bitmask8( PB2 );}
    bool _getSS() { return PINB & Bitmask8( PB2 ); }

    State _state, _prevState;
    uint8_t _progress;
    uint8_t _limit;
    uint8_t _header[ 5 ];
    Blob _buffer;
    bool _received;
    bool _interrupt;
};

SpiInterface spiInterface;

ISR (SPI_STC_vect) {
    spiInterface.onByte();
}

void onSpiEnd() {
    spiInterface.onSpiEnd();
}

template < uint8_t qSize >
struct BlobQueue {
    BlobQueue(): _size( 0 ) {}

    bool empty() { return _size == 0; }
    uint8_t size() { return _size; }
    Blob& front() { return _q[ 0 ]; }

    Blob pop() {
        Blob b = move( front() );
        for ( uint8_t i = 1; i != qSize; i++ )
            _q[ i - 1 ] = move( _q[ i ] );
        _size--;
        return b;
    }

    void push( Blob&& b ) {
        if ( _size == qSize )
            return;
        _q[ _size++ ] = move( b );
    }
private:
    int _size;
    Blob _q[ qSize ];
};

enum InterruptFlag {
    CONNECT = 1 << 0,
    BLOB = 1 << 1
};

class D2DInterface {
public:
    D2DInterface( uint32_t baudrate ):
        _intMask( InterruptFlag::CONNECT | InterruptFlag::BLOB ),
        _intFlags( 0 )
    {
        UBRR0 = _prescaler( baudrate );
        UCSR0B = Bitmask8( RXEN0, TXEN0, RXCIE0 );
        UCSR0C = Bitmask8( UCSZ00, UCSZ01 );
    }

    void tick() {
        _receive();
        _send();
    }

    uint8_t available() { return _inQ.size(); }
    uint8_t pending() { return _outQ.size(); }
    // 2 bytes leading zeroes, 2 bytes content type, 2 bytes size, payload
    Blob receive() { return move( _inQ.pop() ); }
    // 2 bytes leading zeroes, 2 bytes content type, 2 bytes size, payload
    void send( Blob&& b ) {
        if ( !b )
            return;
        _outQ.push( move( b ) );
    }

    void interruptMask( uint16_t mask ) { _intMask = mask; }
    uint16_t readInterrupts() {
        InterruptGuard _;
        uint16_t r = _intFlags;
        _intFlags = 0;
        return r;
    }

    void _onRx() {
        _receive( UDR0 );
    }

// private:
    void _receive() {
        if ( !_in )
            _resetReceive();
        if ( _inProgress > MAX_BUFFER_SIZE ) {
            _resetReceive();
            return;
        }
        if ( _inProgress > 6u ) {
            uint16_t size = _in.as< uint16_t >( 4 ) + 10u;
            if ( _inProgress >= size ) {
                // ToDo: Check CRC
                Blob oldBlob = move( _in );
                uint8_t oldProgress = _inProgress;

                _resetReceive();
                for ( uint8_t i = size; i != oldProgress; i++ )
                    _receive( oldBlob[ i ] );

                _inQ.push( move( oldBlob ) );

                if ( true || _intMask & InterruptFlag::BLOB ) {
                    _intFlags |= InterruptFlag::BLOB;
                    spiInterface.interruptMaster();
                }
            }
        }
    }

    void _send() {
        if ( !_out ) {
            if ( _outQ.empty() )
                return;
            _out = move( _outQ.pop() );
            _outProgress = 0;
            _out.as< uint8_t >( 0 ) = 0xAA; // header
            _out.as< uint8_t >( 1 ) = 0;    // reserved
            uint8_t size = _out.as< uint16_t >( 4 );
            _out.as< uint32_t >( 6 + size ) = 0; // ToDo: CRC
            if ( size > MAX_BUFFER_SIZE ) {
                _out.free();
                return;
            }
        }

        uint8_t size = _out.as< uint16_t >( 4 ) + 10;
        for ( uint8_t i = 0; i != 10; i++ ) {
            if ( _outProgress == size ) {
                _out.free();
                return;
            }
            _transmit( _out[ _outProgress++ ] );
        }
    }

    void _transmit( char c ) {
        while( _txBusy() );
        UDR0 = c;
    }

    D2DInterface& _print( const char *text ) {
        while ( *text ) {
            _transmit( *text++ );
        }
        return *this;
    }

    D2DInterface& _print( int x ) {
        char buff[20];
        sprintf( buff, "%d", x );
        return _print( buff );
    }

    void _receive( char c ) {
        if ( !_in )
            return;
        _in[ _inProgress++ ] = c;
        // Check header
        if ( _inProgress == 1 && _in[ 0 ] != 0XAA )
            _inProgress = 0;
    }

    void _resetReceive() {
        _in = allocate( MAX_BUFFER_SIZE );
        _inProgress = 0;
    }

    static constexpr uint16_t _prescaler( uint32_t baudrate ) {
        return ( F_CPU / baudrate / 16 ) - 1ul;
    }

    bool _txBusy() {
        return !( UCSR0A & Bitmask8( UDRE0 ) );
    }

    BlobQueue< 10 > _inQ;
    BlobQueue< 10 > _outQ;
    Blob _in;
    uint8_t _inProgress;
    Blob _out;
    uint8_t _outProgress;

    uint16_t _intMask;
    uint16_t _intFlags;
};
D2DInterface d2dInterface( 250000 );

ISR( USART_RX_vect ) {
    d2dInterface._onRx();
}

void onVersionCommand( uint8_t* ) {
    auto b = allocate( 4 );
    if ( !b )
        spiInterface.send( Blob(), 0 );
    b.as< uint16_t >( 0 ) = 0;  // Dock variant
    b.as< uint16_t >( 2 ) = 0; // Protocol revision
    spiInterface.send( move( b ), 4 );
}

void onStatusCommand( uint8_t* ) {
    // Arduino dock ignores commands in header
    // ...and sends dummy response
    auto b = allocate( 12 );
    if ( !b )
        spiInterface.send( Blob(), 0 );
    b.as< uint16_t >( 0 ) = 0;  // bitmask
    b.as< uint8_t >( 2 ) = d2dInterface.pending();  // blobs pending to send
    b.as< uint8_t >( 3 ) = d2dInterface.available();  // blobs pending to retrieve
    b.as< uint16_t >( 4 ) = 0; // int voltage
    b.as< uint16_t >( 6 ) = 0; // int current
    b.as< uint16_t >( 8 ) = 0; // ext voltage
    b.as< uint16_t >( 10 ) = 0; // ext current
    spiInterface.send( move( b ), 12 );
}

void onInterruptCommand( uint8_t* header ) {
    uint16_t intMask = as< uint16_t >( header + 1 );
    auto b = allocate( 2 );
    if ( !b )
        spiInterface.send( Blob(), 0 );
    b.as< uint16_t >( 0 ) = d2dInterface.readInterrupts();
    d2dInterface.interruptMask( intMask );
    spiInterface.send( move( b ), 2 );
}

void onSendBlobCommand( uint8_t* ) {
    spiInterface.receive( 2 );
}

void onReceiveBlobCommand( uint8_t* ) {
    if ( d2dInterface.available() == 0 ) {
        auto b = allocate( 4 );
        if ( !b )
            spiInterface.send( Blob(), 0 );
        b.as< uint16_t >( 0 ) = 0;
        b.as< uint16_t >( 2 ) = 0;
        spiInterface.send( move( b ), 4 );
        return;
    }

    Blob b = d2dInterface.receive();
    spiInterface.send( move( b ), MAX_BUFFER_SIZE, 2 );
}

template < typename T >
D2DInterface& _print( T t ) {
    return d2dInterface._print( t );
}

void onAllocFail() {
    d2dInterface._inQ.pop();
    spiInterface.interruptMaster();
}

int main() {
    sei();

    spiInterface.interruptMaster();

    bool prevSs;
    while( true ) {
        if ( spiInterface.headerReceived() ) {
            auto command = static_cast< SpiCommand >( spiInterface.header()[ 0 ] );
            switch ( command ) {
            case SpiCommand::VERSION:
                onVersionCommand( spiInterface.header() ); break;
            case SpiCommand::STATUS:
                onStatusCommand( spiInterface.header() ); break;
            case SpiCommand::INTERRUPT:
                onInterruptCommand( spiInterface.header() ); break;
            case SpiCommand::SEND_BLOB:
                onSendBlobCommand( spiInterface.header() ); break;
            case SpiCommand::RECEIVE_BLOB:
                onReceiveBlobCommand( spiInterface.header() ); break;
            default:
                break;
            }
        }

        if ( spiInterface.blobReceived() ) {
            d2dInterface.send( move( spiInterface.getBlob() ) );
        }

        d2dInterface.tick();

        bool ss = spiInterface._getSS();
        if ( !prevSs && ss ) {
            spiInterface.onSpiEnd();
        }
        prevSs = ss;
    }
}
