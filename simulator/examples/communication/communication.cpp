#include <iostream>

#include <lwip/tcpip.h>

#include "rofi_hal.hpp"
#include "roif6.hpp"

int main()
{
    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );
	sleep( 1 );

    std::cout << "ID: " + std::to_string( rofi::hal::RoFI::getLocalRoFI().getId() ) + "\n";

	_rofi::RoIF6 roif( rofi::hal::RoFI::getLocalRoFI() );
	roif.setUp();
	roif.printAddresses();

    rofi::hal::RoFI::getLocalRoFI().getConnector( 1 ).connect();

	while( true ) {
		sleep( 2 );
		roif.printTable();
	}

    std::cout << "Ending communication example\n";
}
