#pragma once

#include <board.hpp>
#include <drivers/adc.hpp>

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


    static PowerManagement& instance() {
        static PowerManagement m;
        return m;
    }
private:
    PowerManagement() = default;

    static float _rDiv( uint32_t measurement, float bottom, float top ) {
        float measurementVoltage = ADC_REF * measurement / ( 1 << ADC_BIT_RES );
        float ratio = ( bottom + top ) / bottom;
        return measurementVoltage * ratio;
    }
};