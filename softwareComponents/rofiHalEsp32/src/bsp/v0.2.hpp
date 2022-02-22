#pragma once

#include <driver/gpio.h>
#include <peripherals/dynamixel.hpp>

namespace rofi::hal::bsp {

using ServoBus = dynamixel::Bus;
using ServoId  = dynamixel::Id;

static const uart_port_t servoBusUart     = UART_NUM_1;
static const gpio_num_t  servoBusPin      = GPIO_NUM_4;
static const int         servoBusBaudrate = 1000000   ;
static const ServoId     alphaId          = 1         ;
static const ServoId     betaId           = 2         ;
static const ServoId     gammaId          = 3         ;
static const float       alphaRatio       = 1.0f      ;
static const float       betaRatio        = 1.0f      ;
static const float       gammaRatio       = 1.0f      ;


inline ServoBus buildServoBus() {
    return ServoBus( servoBusUart, servoBusPin, servoBusBaudrate );
}

inline std::string errorName( dynamixel::Error e ) {
    return dynamixel::errorName( e );
}

inline std::string errorName( dynamixel::HardwareError e ) {
    return dynamixel::errorName( e );
}


}; // rofi::hal::bsp