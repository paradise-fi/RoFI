#include <rofi_hal.hpp>
#include <firmware_updater.hpp>
#include "esp_log.h"
#include <iostream>
#include <thread>
#include <driver/gpio.h>


extern "C" void app_main() {
    while(1) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );

    }
}
