#include <iostream>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <peripherals/herculex.hpp>

extern "C" void app_main() {
    using namespace std::chrono_literals;

    Bus bus( UART_NUM_1, GPIO_NUM_4, GPIO_NUM_2 );
    try {
        for ( int i = 0; i != 10; i++ ) {
            auto servo = bus.getServo( 0xFD );
            servo.torqueOn();
            servo.move( Angle::deg( -90 ), 2000ms );
            vTaskDelay( 2000 / portTICK_PERIOD_MS );
            servo.move( Angle::deg( 90 ), 2000ms );
            vTaskDelay( 2000 / portTICK_PERIOD_MS );
        }
    }
    catch( const std::runtime_error& e ) {
        std::cerr << "Error: " << e.what() << "\n";
        return;
    }
    while( true ) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }


    while( true ) {
        bool found = false;
        std::cout << "Searching for servos...\n";
        for ( int i = 0; i != 0xFE; i++ ) {
            auto servo = bus.getServo( i );
            if ( servo.active() ) {
                found = true;
                std::cout << std::hex << "  0x" << i << "(" << std::dec << i << ")\n";
            }
        }
        if ( !found )
            std::cout << "No servos found\n";

        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}