#pragma once

#include <cassert>
#include <drivers/timer.hpp>
#include <drivers/gpio.hpp>

class Motor {
public:
    Motor( Timer::Pwm pwm, Gpio::Pin pin )
        : _pwm( pwm ), _pin( pin )
    {
        set( 0 );
    }

    void enable() {
        _pin.setupPPOutput();
        _pwm.enable();
    }

    void disable() {
        _pin.setupAnalog( false );
        _pwm.disable();
    }

    void set( int val ) {
        assert( val >= -100 && val <= 100 );
        if ( val < 0 ) {
            int setpoint = _pwm.top() + val * _pwm.top() / 100;
            _pwm.set( setpoint );
            _pin.write( true );
        }
        else {
            int setpoint = val * _pwm.top() / 100;
            _pwm.set( setpoint );
            _pin.write( false );
        }
    }
private:
    Timer::Pwm _pwm;
    Gpio::Pin _pin;
};