#pragma once

#include <cassert>
#include <array>
#include <stm32g0xx_hal.h>
#include <drivers/timer.hpp>
#include <drivers/gpio.hpp>
#include <configuration.hpp>
#include <system/dbg.hpp>

#include "jammonitor.hpp"

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

    void stop(){
      set(0);
    }
private:
    Timer::Pwm _pwm;
    Gpio::Pin _pin;
};

namespace SliderShape {
static const float kRetractedPosition = 0.0;
static const float kExpandedPosition = 1.0;
}

float getDirection(float position, float goal);
float getPower(float position, float goal_position);

class Slider {
public:
    Slider( Motor motor, const std::array< Gpio::Pin, cfg::motorSensorsCount > & positionPins )
        : _motor( std::move( motor ) ),
          _positionPins( positionPins ),
          _currentState( State::Unknown )
    {
        motor.stop();
        motor.enable();

        for ( auto posPin : _positionPins ) {
            posPin.setupInput( false );
        }
    }

    enum class State {
      Unknown,
      Retracted,
      Expanding,
      Expanded,
      Retracting,
      Stopped,
      jammed};

    void expand() {
        _goal = State::Expanded;
        _goalPosition = SliderShape::kExpandedPosition;
        _jamMonitor.start(_position(), _goalPosition);
        _currentState = State::Expanding;
    }

    void retract() {
        _goal = State::Retracted;
        _goalPosition = SliderShape::kRetractedPosition;
        _jamMonitor.start(_position(), _goalPosition);
        _currentState = State::Retracting;
    }

    void stop() {
        _goal = State::Stopped;
        _currentState = State::Stopped;
        _motor.stop();
    }

    bool moveFinished( float pos) const{
      switch(_goal) {
      case State::Expanded:
        return pos >= _goalPosition;
      case State::Retracted:
        return pos <= _goalPosition;
      default:
        return true;
      }
    }


    void run() {
        float pos = _position();
        dbg_log(pos);
        dbg_log((int)_goal);

        if( _goal == State::Expanded || _goal == State::Retracted){
            if(moveFinished(pos)){
              _currentState = _goal;
              _goal = State::Stopped;
              _motor.stop();
              return;
            }

            switch ( _jamMonitor.update(pos)) {
            case JamStatus::Nominal:
              _motor.set(100 * getPower(pos, _goalPosition));
              return;
            case JamStatus::Jammed:
              _jamMonitor.startRecovery();
              [[fallthrough]];
            case JamStatus::Recovery:
              _motor.set(100 * getDirection(_goalPosition, pos));
              return;

            case JamStatus::Fatal:
              _currentState = State::jammed;
            }
        }

        _motor.stop();
    }

    State getGoal() { return _goal; }

    /**
     * Computes position of the connector as value <0.0,1.0>.
     * 0.0 means fully retracted, 1.0 represents fully extended/locked
     *
     * Note: when fully retracted no sensor is actually triggered,
     *
     * @return
     */
    float _position() const{
      auto readCount = 0;
      float readPosSum = 0;


      for( size_t  pos_idx = 0; pos_idx < std::size(_positionPins); pos_idx++ ){
        if ( ! _positionPins[pos_idx].read() ) {
          ++readCount;
          readPosSum += pos_idx + 1;
        }
      }

      if( readCount == 0){
        return 0.0;
      }

      auto average_pos = readPosSum / readCount;
      return  average_pos / _positionPins.size();
    }

    Motor _motor;
    JamMonitor _jamMonitor;

    const std::array< Gpio::Pin, cfg::motorSensorsCount > _positionPins;

    State _goal = State::Stopped;
    float _goalPosition;

    State _currentState = State::Unknown;
};



