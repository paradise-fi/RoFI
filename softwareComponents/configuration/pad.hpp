#pragma once

#include <array>
#include <iostream>
#include "rofibot.hpp"

namespace rofi::configuration {

class Pad : public Module {
    Module buildPad( int sizeN, int sizeM );
    Module buildPad( int size );

public:
    Pad( int sizeN, int sizeM ) : Module( buildPad( sizeN, sizeM ) ) {};

    explicit Pad( int size ) : Module( buildPad( size ) ) {};

    virtual ~Pad() = default;
};


} // namespace rofi::configuration
