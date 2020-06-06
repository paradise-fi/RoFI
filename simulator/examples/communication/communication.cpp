#include <iostream>

#include <lwip/tcpip.h>

#include "rofi_hal.hpp"

int main()
{
    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );

    std::cout << "ID: " + std::to_string( rofi::hal::RoFI::getLocalRoFI().getId() ) + "\n";
    rofi::hal::RoFI::getLocalRoFI().getConnector( 1 ).connect();

    std::cout << "Ending communication example\n";
}
