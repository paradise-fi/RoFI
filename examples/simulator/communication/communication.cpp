#include <iostream>

#include <lwip/tcpip.h>

#include "udp6Example.hpp"


int main()
{
    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );

    int id = rofi::hal::RoFI::getLocalRoFI().getId();
    std::cout << "ID: " + std::to_string( id ) + "\n";

    rofinet::RoIF roif( rofi::hal::RoFI::getLocalRoFI() );
    roif.setUp();

    if ( id == 1 )
    {
        udpEx6::runMaster();
    }
    else
    {
        udpEx6::runSlave( "fc07::1:0:0:1" );
    }

    std::cout << "Ending communication example\n";
}
