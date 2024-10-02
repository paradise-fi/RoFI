#pragma once

#include <cassert>
#include <optional>

#include <system/dbg.hpp>

#include <configuration.hpp>
#include "motor.hpp"

#include <drivers/gpio.hpp>
#include <drivers/i2c.hpp>
#include <drivers/spi.hpp>
#include <drivers/timer.hpp>
#include <drivers/uart.hpp>

extern Dbg &dbgInstance();

extern void SystemCoreClockUpdate(void);

/**
 * This is Board Support Package integration into RoFI.
 * 
 * It's main purpose is to specify objects, functions, ... 
 * that are needed for given firmware in a way which enables 
 * better and easier portability for different hardware boards.
 * 
 * 
*/
namespace bsp {
    extern const Gpio::Pin connectorSenseA;
    extern const Gpio::Pin connectorSenseB;
    extern const Gpio::Pin internalSwitchPin;
    extern const Gpio::Pin internalVoltagePin;
    extern const Gpio::Pin internalCurrentPin;
    extern const Gpio::Pin externalSwitchPin;
    extern const Gpio::Pin externalVoltagePin;
    extern const Gpio::Pin externalCurrentPin;


    extern const Gpio::Pin spiCSPin;
    extern const Gpio::Pin lidarEnablePin;
    extern const Gpio::Pin lidarIRQPin;

    extern const std::array< Gpio::Pin, cfg::motorSensorsCount > posPins;


    extern std::optional< Timer > motorTimer;
    extern std::optional< Timer::Pwm > motorPwm;
    extern std::optional< Motor > sliderMotor;
    extern std::optional< Spi > moduleComm;
    extern std::optional< Uart > connectorComm;

    extern std::optional< I2C > i2c;

    /**
     * Function to setup your board, MUST be called before using 
     * any members of `bsp` namespace. Setups board specific
     * clock, timers, pins, peripherals, etc.
     * 
     * Should be implemented in `bsp.cpp` with board specific 
     * initialization.
    */
    void setupBoard();
}
