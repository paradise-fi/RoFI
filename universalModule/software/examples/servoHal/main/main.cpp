#include <iostream>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <peripherals/herculex.hpp>
#include <rofi_hal.hpp>

extern "C" void app_main() {
    using namespace std::chrono_literals;
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