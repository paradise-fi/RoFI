#include "uart.hpp"

std::array< Uart::Handlers, Uart::availablePeripherals.size() > Uart::_uarts;
