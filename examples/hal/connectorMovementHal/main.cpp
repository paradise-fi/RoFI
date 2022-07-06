#include <iostream>
#include <sstream>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <rofi_hal.hpp>

extern "C" void app_main() {
    try {
        std::cout << "Program starts\n";
        auto localRoFI = rofi::hal::RoFI::getLocalRoFI();
        std::cout << "Got local RoFI\n";
        auto conn2 = localRoFI.getConnector( 1 );

        char counter = 0;
        while( true ) {
            counter++;
            std::cout << "Extending\n";
            conn2.connect();
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
            std::cout << "Retracting\n";
            conn2.disconnect();
            vTaskDelay( 3000 / portTICK_PERIOD_MS );
        }


    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }

    while ( true ) {
            vTaskDelay( 1000 / portTICK_PERIOD_MS );
        }
}
