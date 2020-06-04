#include <cassert>
#include <future>
#include <iostream>

#include <lwip/tcpip.h>

#include "rofi_hal.hpp"

int main()
{
    tcpip_init( nullptr, nullptr );
}
