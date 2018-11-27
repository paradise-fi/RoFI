#include <iostream>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>

#include <freertos/task.h>
#include <vector>
#include <tuple>

#include "dock.hpp"
#include "freeRTOS.hpp"

#include <tcpip_adapter.h>

extern "C" void app_main() {
    tcpip_adapter_init();

    gpio_install_isr_service( 0 );

    spi_bus_config_t spiBus = SpiBusConfig()
        .mosi( 5 )
        .miso( 18 )
        .sclk( 19 );
    auto r = spi_bus_initialize( HSPI_HOST, &spiBus, 1 );
    ESP_ERROR_CHECK( r );

    _rofi::Dock d1( HSPI_HOST, GPIO_NUM_14 );
    _rofi::Dock d2( HSPI_HOST, GPIO_NUM_12 );

    d2.onReceive([]( _rofi::Dock&, int, _rofi::PBuf&& buf ) {
        std::cout << "Received: ";
        for ( int i = 0; i != buf.size(); i++ ) {
            std::cout << buf[ i ];
        }
        std::cout << "\n";
    } );

    int counter = 0;
    while( true ) {
        std::cout << "Sending blob\n";

        auto message = _rofi::PBuf( 4 );
        message[ 0 ] = 'A' + ((counter++) % 26);
        message[ 1 ] = 'b';
        message[ 2 ] = 'c';
        message[ 3 ] = 'd';
        d1.sendBlob( 0, std::move( message ) );

        vTaskDelay( 2000 / portTICK_PERIOD_MS );
    }
}