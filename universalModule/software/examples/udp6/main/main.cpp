#include <driver/gpio.h>
#include "esp_netif.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "udp6Example.hpp"


void setupStack() {
    esp_netif_init();
}

const char* getOwn( int id ) {
	if ( id == 3 ) {
		return "fe80:0:0:0:32ae:a4ff:fe15:2544";
	} else if ( id == 1 )
		return "fe80:0:0:0:32ae:a4ff:fe14:e5cc";
	return "fe80:0:0:0:32ae:a4ff:fe17:a888";
}

const char* buildAddress ( int id ) {
	std::ostringstream s;
	s << "fc07::" << id;
	return s.str().c_str();
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
		
		std::cout << "Mac Address: ";
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

std::vector< gpio_num_t > docks( int id ) {
	if ( id == 1 && id == 11 )
		return { GPIO_NUM_14, GPIO_NUM_27 };
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
		_rofi::Ip6Addr( buildAddress( getId() ) ),
		mac(),
		docks( getId() ) );

	roif.setUp();
	roif.printAddresses();
	if ( getId() == 11 ) {
		//roif.addAddress( _rofi::Ip6Addr( "fe80:0:0:0:ae30:0:8083:10a4" ) );
		                                 // "fe80::1" ) );
		udpEx6::runMaster();
	} else
		udpEx6::runSlave(  // "fe80:0:0:0:ae30:0:8083:10a4"
						   "fc07::11"
						);
	
	while ( true ) vTaskDelay( 2000 / portTICK_PERIOD_MS );
}
