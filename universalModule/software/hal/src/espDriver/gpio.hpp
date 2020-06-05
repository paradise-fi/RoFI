#pragma once

#include <iostream>
#include <driver/gpio.h>

namespace rofi::esp32 {

class Gpio {
public:
    Gpio() {}
    operator gpio_config_t() const { return _s; }
    Gpio& pinBitMask( uint64_t p ) { _s.pin_bit_mask = p; return *this; }
    Gpio& mode( gpio_mode_t p ) { _s.mode = p; return *this; }
    Gpio& pullUpEn( gpio_pullup_t p ) { _s.pull_up_en = p; return *this; }
    Gpio& pullDownEn( gpio_pulldown_t p ) { _s.pull_down_en = p; return *this; }
    Gpio& intrType( gpio_int_type_t p ) { _s.intr_type = p; return *this; }
private:
    gpio_config_t _s;
};

std::ostream& operator<<( std::ostream& o, gpio_config_t s );

} // namespace rofi::esp32
