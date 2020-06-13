#include <iostream>

#include <lwip/tcpip.h>

#include "rofi_hal.hpp"
#include "udp6Example.hpp"


int main()
{
    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );
	sleep( 1 );

	int id = rofi::hal::RoFI::getLocalRoFI().getId();
    std::cout << "ID: " + std::to_string( id ) + "\n";

	_rofi::RoIF6 roif( rofi::hal::RoFI::getLocalRoFI() );
	roif.setUp();
	roif.printAddresses();

    rofi::hal::RoFI::getLocalRoFI().getConnector( 1 ).connect();

	if ( id == 1 )
		udpEx6::runMaster();
	else
		udpEx6::runSlave( "fc07::1" );

    std::cout << "Ending communication example\n";
}
