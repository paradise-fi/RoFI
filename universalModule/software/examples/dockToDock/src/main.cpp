#include <iostream>
#include <sstream>

#include <dock.hpp>

extern "C" void app_main() {
    using namespace _rofi;

    gpio_install_isr_service( ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM );
    spi_bus_config_t spiBus = SpiBusConfig()
        .mosi( 5 )
        .miso( 18 )
        .sclk( 19 );
    auto r = spi_bus_initialize( HSPI_HOST, &spiBus, 1 );
    ESP_ERROR_CHECK( r );

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