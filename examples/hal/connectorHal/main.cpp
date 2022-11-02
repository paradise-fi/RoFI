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
    try {
        auto localRoFI = rofi::hal::RoFI::getLocalRoFI();
        std::cout << "Got local RoFI\n";
        auto conn1 = localRoFI.getConnector( 0 );

        conn1.onPacket( printPacket );

        char counter = 0;
        while( true ) {
            counter++;
            const char* msg = "Hello from 1! ";
            const int len = strlen( msg );
            auto buf = rofi::hal::PBuf::allocate( len );
            for ( int i = 0; i != len; i++ )
                buf[ i ] = msg[ i ];
            buf[ len - 1 ] = 'a' + ( counter ) % 26;
            conn1.send( 42, std::move( buf ) );
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
        }

        while ( true ) {
            vTaskDelay( 1000 / portTICK_PERIOD_MS );
        }
    }
    catch ( const std::runtime_error& e ) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
