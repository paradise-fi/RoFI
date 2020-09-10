#include <iostream>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <peripherals/herculex.hpp>
#include <rofi_hal.hpp>

void printPacket( rofi::hal::Connector, uint16_t contentType, rofi::hal::PBuf packet ) {
    std::cout << "Packet! ***************************************************************************************\n";
    std::cout << "Content type: " << contentType << ": ";
    for ( auto it = packet.chunksBegin(); it != packet.chunksEnd(); ++it ) {
        for ( int i = 0; i != it->size(); i++ )
            std::cout << it->mem()[ i ];
    }
    std::cout << "\n";
}

extern "C" void app_main() {
    std::cout << "Program starts\n";
    auto localRoFI = rofi::hal::RoFI::getLocalRoFI();
    std::cout << "Got local RoFI\n";
    auto conn1 = localRoFI.getConnector( 0 );
    auto conn2 = localRoFI.getConnector( 1 );

    conn1.onPacket( printPacket );
    conn2.onPacket( printPacket );

    char counter = 0;
    while( true ) {
        std::cout << "Send\n";
        counter++;
        const char* msg = "Hello from 1! ";
        const int len = strlen( msg );
        auto buf = rofi::hal::PBuf::allocate( len );
        for ( int i = 0; i != len; i++ )
            buf[ i ] = msg[ i ];
        buf[ len - 1 ] = 'a' + ( counter ) % 26;
        conn1.send( 42, std::move( buf ) );

        const char* msg2 = "Hello form 2! ";
        const int len2 = strlen( msg2 );
        auto buf2 = rofi::hal::PBuf::allocate( len2 );
        for ( int i = 0; i != len2; i++ )
            buf2[ i ] = msg2[ i ];
        buf2[ len - 1 ] = 'a' + ( counter ) % 26;
        conn2.send( 42, std::move( buf2 ) );

        vTaskDelay( 3000 / portTICK_PERIOD_MS );
    }

    while ( true ) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}