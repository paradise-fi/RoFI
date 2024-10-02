#pragma once

#include <drivers/adc.hpp>
#include <drivers/timer.hpp>

#include <board.hpp>

class PowerManagement {
public:
    void setup();

    float getUsbVoltage();
    float getBusVoltage();
    float getBattVoltage();

    PowerManagement& enableUsbToBus( bool val = true ) {
        PM_USB_TO_BUS_EN_PIN.write( val );
        return *this;
    }

    PowerManagement& enableBattToBus( bool val = true ) {
        PM_BATT_TO_BUS_EN_PIN.write( val );
        return *this;
    }

    PowerManagement& enableCharging( bool val = true ) {
        PM_CHG_EN_PIN.write( val );
        return *this;
    }

    void shutdown() {
        PM_SHUTDOWN_PIN.write( false );
    }

    bool isCharging() {
        return !PM_CHG_OK_PIN.read();
    }

    void beepStart( int freq, uint8_t power ) {
        _timer.setup( FreqAndRes( freq, 512 ) );
        _timer.enable();
        _pwm.attachPin( BUZZER_PIN_A );
        _pwm.attachPin( BUZZER_PIN_B );
        _pwm.set( power );
        _pwm.enable();
    }

    void beepStop() {
        _pwm.disable();
        _timer.disable();
        BUZZER_PIN_A.setupInput( false );
        BUZZER_PIN_B.setupInput( false );
    }

    static PowerManagement& instance() {
        static PowerManagement m;
        return m;
    }
private:
    PowerManagement(): _timer( BUZZER_TIM ) {
        _pwm = _timer.pwmChannel( BUZZER_CHAN );
    }

    static float _rDiv( uint32_t measurement, float bottom, float top ) {
        float measurementVoltage = ADC_REF * measurement / ( 1 << ADC_BIT_RES );
        float ratio = ( bottom + top ) / bottom;
        return measurementVoltage * ratio;
    }

    Timer _timer;
    Timer::Pwm _pwm;
};
