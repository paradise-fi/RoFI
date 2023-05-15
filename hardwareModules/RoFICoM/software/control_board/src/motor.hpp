#pragma once

#include <cassert>
#include <array>
#include <stm32g0xx_hal.h>
#include <drivers/timer.hpp>
#include <drivers/gpio.hpp>
#include <configuration.hpp>

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
    Slider( Motor motor, const std::array< Gpio::Pin, cfg::motorSensorsCount > & positionPins )
        : _motor( std::move( motor ) ),
          _positionPins( positionPins ),
          _goal( State::Retracted ),
          _currentState( State::Unknown )
    {
        motor.set( 0 );
        motor.enable();

        for ( auto posPin : _positionPins ) {
            posPin.setupInput( false );
        }
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

    void stop() {
        _goal = State::Unknown;
        _currentState = State::Unknown;
        _onStateChange();
        _move();
    }

    void run() {
        const auto pos = _position();
        // TODO:
        if ( pos == -1 ) {
            return;
        }
        if ( _goal == State::Retracted ) {
            if ( pos - _endThreshold <= _retractedPosition )
                _set( State::Retracted );
            else
                _set( State::Retracting );
        }
        else if ( _goal == State::Expanded ) {
            if ( ( pos + _endThreshold >= _expandedPosition ) )
                _set( State::Expanded );
            else
                _set( State::Expanding );
        }
        _move();
    }

    State getGoal() { return _goal; }

// TODO: private:
    void _move() {
        const int MAX_POWER = 100;
        const int pos = _position();
        if ( _currentState == State::Expanding )
            _motor.set( _coef( pos ) * -MAX_POWER );
        else if ( _currentState == State::Retracting )
            _motor.set( _coef( pos ) * MAX_POWER );
        else
            _motor.set( 0 );
    }

    void _set( State s ) {
        if ( s != _currentState )
            _onStateChange();
        _currentState = s;
    }

    void _onStateChange() {
        switch (_goal)
        {
        case State::Retracting:
        case State::Retracted:
            _goalPosition = _retractedPosition;
            break;
        case State::Expanding:
        case State::Expanded:
            _goalPosition = _expandedPosition;
            break;
        default:
            break;
        }
    }

    float _coef( int position ) {
        const int threshold = 50;
        const int positionFromGoal = std::abs( position - _goalPosition );
        if ( positionFromGoal <= _endThreshold ) {
            return 0;
        } else if ( positionFromGoal <= threshold ) {
            const float min_coef = 0.5f;
            float coef = float(positionFromGoal) / 100 / 2; // + 0.25f;
            return coef; // std::max(coef, min_coef);
        }
        return 1;
    }

    int _position() {
        auto readCount = 0;
        auto readPosSum = 0;
        for ( auto i = 0; auto posPin : _positionPins ) {
            if ( ! posPin.read() ) {
                ++readCount;
                readPosSum += i;
            }
            ++i;
        }
        return 
        readCount == 0
        ? -1
        : ( 100 * readPosSum / ( readCount ) )  / ( _positionPins.size() - 1 );
    }

    Motor _motor;
    const std::array< Gpio::Pin, cfg::motorSensorsCount > /* Error with cycle referencing?: decltype( bsp::posPins )*/ & _positionPins;
    State _goal;
    State _currentState;
    uint8_t _goalPosition;
    const uint8_t _retractedPosition = 25;
    const uint8_t _expandedPosition = 100;
    const uint8_t _endThreshold = 30;

};
