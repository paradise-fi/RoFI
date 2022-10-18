#include "espDriver/gpio.hpp"

std::ostream& operator<<( std::ostream& o, gpio_config_t s ) {
    o << "pin_bit_mask: " << s.pin_bit_mask << "\n";
    o << "mode: " << s.mode << "\n";
    o << "pull_up_en: " << s.pull_up_en << "\n";
    o << "pull_down_en: " << s.pull_down_en << "\n";
    o << "intr_type: " << s.intr_type << "\n";
    return o;
}
