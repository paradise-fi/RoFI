#include <lwip/tcpip.h>

#include "rofi_hal.hpp"

int main()
{
    tcpip_init( nullptr, nullptr );

    rofi::hal::RoFI::getLocalRoFI().getConnector( 1 ).connect();
}
