#include <firmware_updater.hpp>
#include <rofi_hal.hpp>
#include "esp_log.h"



extern "C" void app_main() {
    while(1) {
        vTaskDelay( 1000 / portTICK_PERIOD_MS );

    }
}
