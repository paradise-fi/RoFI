#include <Arduino.h>
#include <EnableInterrupt.h>

#include "blob.hpp"
#include "modernHack.hpp"

volatile int status;
volatile int arg;

struct SpiInterface {
private:
    enum class State { HEADER, RECEIVE, SEND };
public:
    SpiInterface(): _buffer( Blob::allocate() ) {
        pinMode( MISO, OUTPUT );
        SPCR = bit( SPE ) | bit(SPIE);
    }

    bool headerReceived() {
        return _state == State::HEADER && _progress == 4;
    }

    uint8_t* header() { return _header; }

    bool blobReceived() {
        return _prevState == State::RECEIVE && _state == State::HEADER;
    }

    Blob&& getBlob() {
        return move( _buffer );
    }

    void receive() {
        nextState( State::RECEIVE );
        _progress = 0;
        _buffer = move( Blob::allocate() );
    }

    void send( Blob& b ) {
        nextState( State::SEND );
        _progress = 0;
        _buffer = move( b );
        shiftOutBuffer();
    }

    void onSpiEnd() {
        SPDR = 0;
        nextState( State::HEADER );
        _progress = 0;
    }

    void onByte() {
        if ( _state == State::SEND ) {
            if ( _buffer )
                shiftOutBuffer();
        } else {
            SPDR = 0;
            if ( _state == State::HEADER ) {
                _header[ _progress++ ] = SPDR;
            }
            else {
                if (_buffer )
                    _buffer[ _progress++ ] = SPDR;
            }
        }
    }
private:
    void shiftOutBuffer() {
        SPDR = _buffer[ _progress++ ];
    }

    void nextState( State s ) {
        _prevState = _state;
        _state = s;
    }

    State _state, _prevState;
    uint8_t _progress;
    uint8_t _header[ 4 ];
    Blob _buffer;
};

SpiInterface spiInterface;

ISR (SPI_STC_vect) {
    SPDR = 42;
    spiInterface.onByte();
}

void onSpiEnd() {
    spiInterface.onSpiEnd();
}

void setup() {
    Serial.begin( 115200 );
    enableInterrupt( 10, onSpiEnd, RISING );
}

void loop() {
    if ( spiInterface.headerReceived() ) {
        auto b = Blob::allocate();
        uint8_t arg = spiInterface.header()[ 0 ];
        for ( uint8_t i = 0; i != 16; i++ )
            b[ i ] = 'a' + arg;
        spiInterface.send( b );
    }
}
