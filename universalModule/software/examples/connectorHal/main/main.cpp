#include <iostream>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <peripherals/herculex.hpp>
#include <rofi_hal.hpp>

extern "C" void app_main() {
    using namespace std::chrono_literals;

    // bool found = false;
    // std::cout << "Searching for servos...\n";
    // rofi::herculex::Bus bus( UART_NUM_1, GPIO_NUM_4, GPIO_NUM_2 );
    // for ( int i = 0; i != 0xFE; i++ ) {
    //     auto servo = bus.getServo( i );
    //     if ( servo.active() ) {
    //         found = true;
    //         std::cout << std::hex << "  0x" << i << "(" << std::dec << i << ")\n";
    //     }
    // }
    // if ( !found )
    //     std::cout << "No servos found\n";
    // bus.send( rofi::herculex::Packet::eepWrite( 0xFD, rofi::herculex::EepRegister::ID, uint8_t( 1 ) ) );
    // bus.send( rofi::herculex::Packet::ramWrite( 0xFD, rofi::herculex::RamRegister::ID, uint8_t( 1 ) ) );
    // std::cout << "Id should be changed\n";

    // std::cout << "Searching for servos...\n";
    // for ( int i = 0; i != 0xFE; i++ ) {
    //     auto servo = bus.getServo( i );
    //     if ( servo.active() ) {
    //         found = true;
    //         std::cout << std::hex << "  0x" << i << "(" << std::dec << i << ")\n";
    //     }
    // }
    // if ( !found )
    //     std::cout << "No servos found\n";

    // vTaskDelay( 1000 / portTICK_PERIOD_MS );
    try {
        auto rof = rofi::hal::RoFI::getLocalRoFI();
        auto joint = rof.getJoint( 0 );
        joint.onError( []( auto, auto, std::string msg ) { std::cout << msg << "\n"; } );
        while ( true ) {
            std::cout << "Moving! 3\n";
            joint.setVelocity( 60 );
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
            std::cout << "Moving! -3\n";
            joint.setVelocity( -60 );
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
        }
        while ( true ) {
            joint.setPosition( -1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone1\n"; } );
            vTaskDelay( 2000 / portTICK_PERIOD_MS );
            joint.setPosition( 1.5, 1.5, []( rofi::hal::Joint ){ std::cout << "\tDone2\n"; } );
            vTaskDelay( 2000 / portTICK_PERIOD_MS );
        }
    } catch ( const std::runtime_error& e ) {
        std::cout << e.what() << "\n";
    }
    while ( true ) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}