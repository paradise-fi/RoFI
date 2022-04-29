#include <iostream>

#include <rofi_hal.hpp>


namespace hal = rofi::hal;

int main()
{
    auto localRoFI = hal::RoFI::getLocalRoFI();
    std::cout << "Hello world from module " << localRoFI.getId() << "!\n";
}
