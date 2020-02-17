#pragma once

#include <iostream>

#include "dock.hpp"

namespace test {

void simpleDocks() {
    using namespace _rofi;

    Dock d( HSPI_HOST, GPIO_NUM_14 );
    d.onReceive( []( Dock& d, int contentType, PBuf&& message ) {
        std::cout << "  Rec: " << message.asString() << "\n";
    } );

    char counter = 0;
    while( true ) {
        std::cout << "Send\n";
        const char* msg = "Hello world!  ";
        const int len = strlen( msg );
        PBuf buf = PBuf::allocate( len );
        for ( int i = 0; i != len; i++ )
            buf[ i ] = msg[ i ];
        buf[ len - 1 ] = 'a' + ( counter++ ) % 26;
        d.sendBlob( 0, std::move( buf ) );

        vTaskDelay( 10 / portTICK_PERIOD_MS );
    }
}

} // namespace test