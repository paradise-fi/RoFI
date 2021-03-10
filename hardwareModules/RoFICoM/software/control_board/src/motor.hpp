#pragma once

#include <cassert>
#include <stm32g0xx_hal.h>
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
        if (val < -100 || val > 100 ) {
            Dbg::info("Invalid motor value %d", val);
        }
        if ( val < -100 )
            val = -100;
        if ( val > 100 )
            val = 100;
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

class Slider {
public:
    Slider( Motor motor, Gpio::Pin retractionLimit, Gpio::Pin expansionLimit )
        : _motor( std::move( motor ) ),
          _retrLimit( retractionLimit ),
          _expLimit( expansionLimit ),
          _goal( State::Retracted ),
          _currentState( State::Unknown )
    {
        _retrLimit.setupInput( true ).invert()
            .setupInterrupt( LL_EXTI_TRIGGER_RISING_FALLING, [&]( bool ) {
                run();
            });
        _expLimit.setupInput( true ).invert()
            .setupInterrupt( LL_EXTI_TRIGGER_RISING_FALLING, [&]( bool ) {
                run();
            });
        motor.set( 0 );
        motor.enable();
    }

    enum class State { Unknown, Retracted, Expanding, Expanded, Retracting };

    void expand() {
        _goal = State::Expanded;
        _onStateChange();
    }

    void retract() {
        _goal = State::Retracted;
        _onStateChange();
    }

    void run() {
        if ( _goal == State::Retracted ) {
            if ( _retrLimit.read() )
                _set( State::Retracted );
            else
                _set( State::Retracting );
        }
        else if ( _goal == State::Expanded ) {
            if ( _expLimit.read() )
                _set( State::Expanded );
            else
                _set( State::Expanding );
        }
        _move();
    }
private:
    void _move() {
        uint32_t duration = HAL_GetTick() - _stateStarted;
        if ( _currentState == State::Expanding )
            _motor.set( _coef( duration ) * -80 );
        else if ( _currentState == State::Retracting )
            _motor.set( _coef( duration ) * 50 );
        else
            _motor.set( 0 );
    }

    void _set( State s ) {
        if ( s != _currentState )
            _onStateChange();
        _currentState = s;
    }

    void _onStateChange() {
        _stateStarted = HAL_GetTick();
    }

    float _coef( int duration ) {
        if ( duration > 6000 )
            return 0;
        duration = duration % 1500;
        if ( duration >= 1000 )
            return 0;
        return ramp( 250, 1000, duration );
        if (duration <= 500 )
            return duration / 500.0;
        else
            return 1.0 - (duration - 500) / 500.0;
    }

    float ramp( int accelTime, int totalTime, int currentTime ) {
        if ( currentTime < accelTime )
            return currentTime / float( accelTime );
        if ( totalTime - currentTime < accelTime )
            return (totalTime - currentTime) / float(accelTime);
        return 1;
    }

    Motor _motor;
    Gpio::Pin _retrLimit;
    Gpio::Pin _expLimit;
    State _goal;
    State _currentState;
    uint32_t _stateStarted;
};