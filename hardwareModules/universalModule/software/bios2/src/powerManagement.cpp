#include "powerManagement.hpp"
#include <drivers/adc.hpp>
#include <system/assert.hpp>

void PowerManagement::setup() {
    PM_ADC_PERIPH.setup();

    PM_SHUTDOWN_PIN.setupInput( false ).write( true ).setupODOutput( false );
    PM_CHG_OK_PIN.setupInput( true, true );
    PM_CHG_EN_PIN.setupPPOutput().write( true );
    PM_BATT_TO_BUS_EN_PIN.setupPPOutput().write( false );
    PM_USB_TO_BUS_EN_PIN.setupPPOutput().write( false ); // After testing, change default state

    PM_BUS_VOLTAGE_PIN.setupAnalog();
    PM_USB_VOLTAGE_PIN.setupAnalog();
    PM_BATT_VOLTAGE_PIN.setupAnalog();

    _timer.enable();
}

float PowerManagement::getUsbVoltage() {
    uint32_t sample = PM_USB_VOLTAGE_PIN.readAnalog();
    return _rDiv( sample, PM_USB_R_DIVIDER_BOT, PM_USB_R_DIVIDER_TOP );
}

float PowerManagement::getBusVoltage() {
    uint32_t sample = PM_BUS_VOLTAGE_PIN.readAnalog();
    return _rDiv( sample, PM_BUS_R_DIVIDER_BOT, PM_BUS_R_DIVIDER_TOP );
}

float PowerManagement::getBattVoltage() {
    uint32_t sample = PM_BATT_VOLTAGE_PIN.readAnalog();
    return _rDiv( sample, PM_BATT_R_DIVIDER_BOT, PM_BATT_R_DIVIDER_TOP );
}
