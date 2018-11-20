#include <Arduino.h>
#include <EnableInterrupt.h>

volatile int status;
volatile int arg;

struct SpiInterface {
    SpiInterface() {
        pinMode( MISO, OUTPUT );
        SPCR = bit( SPE ) | bit(SPIE);
        _buffer = _buffers[ 0 ];
    }

    void onSpiEnd() {
        SPDR = 0;
        _state = State::HEADER;
        _progress = 0;
    }

    void onByte() {
        if ( _state == State::SEND ) {
            shiftOutBuffer();
        } else {
            SPDR = 0;
            if ( _state == State::HEADER ) {
                _header[ _progress++ ] = SPDR;
                if ( _progress == 4 )
                    onHeader();
            }
            else {
                _buffer[ _progress++ ] = SPDR;
                onExt();
            }
        }
    }

    void onHeader() {
        for ( int i = 0; i != 16; i++ )
            _buffer[ i ] = 'A' + _header[ 0 ] + i;

        _state = State::SEND;
        _progress = 0;

        shiftOutBuffer();
    }

    void onExt() { }

    void shiftOutBuffer() {
        SPDR = _buffer[ _progress++ ];
    }

    enum class State { HEADER, RECEIVE, SEND };

    State _state;
    uint8_t _progress;
    uint8_t _header[ 4 ];
    uint8_t _buffers[2][255];
    uint8_t *_buffer;
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

int lstat;
void loop() {
    // if ( status != lstat ) {
    //     Serial.print( "Event: ");
    //     Serial.print( status );
    //     Serial.print(": ");
    //     Serial.println( arg );
    //     lstat = status;
    // }
    // if ( status == 43 ) {
    //     for ( int i = 0; i != 4; i++ ) {
    //         Serial.print( spiInterface._header[ i ], DEC );
    //         Serial.print( " " );
    //     }
    //     Serial.println("");
    // }
    // if ( end ) {
    //     Serial.print("\treceived: ");
    //     Serial.println(rx);
    //     rx[0] = rx[1] = rx[2] = rx[3] = '_';
    //     end = false;
    // }
}
