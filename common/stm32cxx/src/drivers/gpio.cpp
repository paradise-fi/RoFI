#include "gpio.hpp"
#include <system/dbg.hpp>

Gpio::Handler Gpio::_lines[ 16 ];

Gpio GpioA( GPIOA );
Gpio GpioB( GPIOB );
#ifdef GPIOC
    Gpio GpioC( GPIOC );
#endif

