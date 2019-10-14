#include "lwipopts.h"
#include <driver/gpio.h>
#include <tcpip_adapter.h>
#include "nvs_flash.h"
#include <esp_event_loop.h>

#include <vector>
#include <tuple>
#include <string>
#include <iostream>
#include <sstream>

#include "udp_6Example.hpp"

// esp32-003 ipv6 fc07:3:3ffb:1a80:0:0:0:3
// esp32-004 ipv6 fc07:4:3ffb:1a80:0:0:0:4

void setupStack() {
    tcpip_adapter_init();
    nvs_flash_init();
}

const char* getOwn( int id ) {
	if ( id == 3 ) {
		return "fe80:0:0:0:32ae:a4ff:fe15:2544";
	} else if ( id == 1 )
		return "fe80:0:0:0:32ae:a4ff:fe14:e5cc";
	return "fe80:0:0:0:32ae:a4ff:fe17:a888";
}

const char* getAddress ( int id ) {
	if ( id == 1 ) {
		return "fe80::1";
	} else if ( id == 3 ) {
		return "fe80::3";
	} else if ( id == 4 ) {
		return "fe80::4";
	} else if ( id == 0 ) {
		return "fe80::0";	
	}
	return "fe80::16";
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
	if ( id == 1 )
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
		mac(),
		//getOwn( getId() ),
		getAddress( getId() ),
		"fe80::",
		"fe80::",
		docks( getId() ) );

	roif.setUp();
	roif.printAddresses();

	if ( getId() == 1 )
		udpEx6::runMaster();
	else
		udpEx6::runSlave( /* "fe80:0:0:0:ae30:0:cce5:14a4" */ "fe80::1" );
	
	while ( true ) vTaskDelay( 2000 / portTICK_PERIOD_MS );
}
