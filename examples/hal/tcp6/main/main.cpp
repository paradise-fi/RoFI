#include <driver/gpio.h>
#include <tcpip_adapter.h>

#include <vector>
#include <tuple>
#include <string>
#include <iostream>
#include <sstream>

#include "tcp6Example.hpp"


void setupStack() {
    tcpip_adapter_init();
}

int getId() {
    static int result = -1;
    if ( result == -1 ) {
        uint8_t macAddress[ 6 ];
        esp_efuse_mac_get_default( macAddress );

        if ( macAddress[ 5 ] == 204 )
            result = 1;
        else if ( macAddress[ 5 ] == 68 )
            result = 3;
        else if ( macAddress[ 5 ] == 136 )
            result = 4;
        else if ( macAddress[ 5 ] == 128 )
            result = 11;
        else if ( macAddress[ 5 ] == 164 )
            result = 12;
        else
            result = 0;

        std::cout << "Mac Adress: ";
        const char* separator = "";
        for ( int x : macAddress ) {
            std::cout << separator << x;
            separator = ":";
        }
        std::cout << "\n";
    }
    return result;
}

_rofi::PhysAddr mac() {
    uint8_t macAddress[ 6 ];
    esp_efuse_mac_get_default( macAddress );
    return _rofi::PhysAddr( macAddress );
}

const char* buildAddress ( int id ) {
	std::ostringstream s;
	s << "fc07::" << id;
	return s.str().c_str();
}

std::vector< gpio_num_t > docks( int id ) {
    if ( id == 1 || id == 3 )
        return { GPIO_NUM_27, GPIO_NUM_14 };
    return { GPIO_NUM_14 };
}

extern "C" void app_main() {
    setupStack();
    getId();

    gpio_install_isr_service( ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM );
    spi_bus_config_t spiBus = SpiBusConfig()
        .mosi( 5 )
        .miso( 18 )
        .sclk( 19 );
    auto r = spi_bus_initialize( HSPI_HOST, &spiBus, 1 );
    ESP_ERROR_CHECK( r );

    _rofi::RoIF6 roif(
		getId(), // same as <IP6Addr( buildAddress( getId() ) ), 128>
		mac(),
		docks( getId() ) );


	if ( getId() == 1 )
		roif.addAddress( _rofi::Ip6Addr( "fc07::a" ), 128 );

	roif.setUp();

	if ( getId() == 1 )
        tcp6Ex::runMaster();
    else
        tcp6Ex::runSlave( "fc07::a" );

    while ( true ) vTaskDelay( 2000 / portTICK_PERIOD_MS );
}
