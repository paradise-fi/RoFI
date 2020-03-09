#pragma once

#include <drivers/gpio.hpp>
#include <drivers/adc.hpp>

// Interface for acessing physical properties of the board
class Phy {
public:
    Phy( const Phy& ) = delete;
    Phy( Phy&& ) = delete;

    static Phy& inst() {
        static Phy phy;
        return phy;
    }

    void setup() {}

    float batteryVoltage() {
        int voltage = _battVoltagePin().readAnalog();
        return dividerInversion( 20, 10 ) * 3.3f * voltage / 4095.f;
    }

    float usbVoltage() {
        int voltage = _usbVoltagePin().readAnalog();
        return dividerInversion( 68, 10 ) * 3.3f * voltage / 4095.f;
    }

    float busVoltage() {
        int voltage = _busVoltagePin().readAnalog();
        return dividerInversion( 68, 10 ) * 3.3f * voltage / 4095.f;
    }

    void enableUsbBridge( bool enable = true ) {
        _enUsbBridgePin().write( enable );
    }

    void enableCharger( bool enable = true ) {
        _enChgPin().write( !enable );
    }

    void enableBattToBusConverter( bool enable = true ) {
        _enBattToBusPin().write( enable );
    }

    void enableUsbToBusConverter( bool enable = true ) {
        _enUsbToBusPin().write( enable );
    }

    Gpio::Pin btnLeft;
    Gpio::Pin btnMid;
    Gpio::Pin btnRight;

private:
    Phy() {
        _setupAnalog();
        _setupButtons();
        _setupGpio();
    }

    void _setupAnalog() {
        Adc1.setup();
        Adc1.calibrate();
        Adc1.enable();

        _battVoltagePin().setupAnalog();
        _usbVoltagePin().setupAnalog();
        _busVoltagePin().setupAnalog();
    }

    void _setupButtons() {
        ( btnLeft = GpioA[ 8 ] ).setupInput( true ).invert();
        ( btnMid = GpioA[ 10 ] ).setupInput( true ).invert();
        ( btnRight = GpioA[ 11 ] ).setupInput( true ).invert();
    }

    void _setupGpio() {
        _enUsbBridgePin().setupPPOutput();
        enableUsbBridge();

        _enChgPin().setupPPOutput();
        _enBattToBusPin().setupPPOutput();
        enableBattToBusConverter( false );
        _enUsbToBusPin().setupPPOutput();
        enableUsbToBusConverter( false );
    }

    static Gpio::Pin _battVoltagePin() {
        return GpioA[ 0 ];
    }

    static Gpio::Pin _usbVoltagePin() {
        return GpioA[ 3 ];
    }

    static Gpio::Pin _busVoltagePin() {
        return GpioA[ 2 ];
    }

    static Gpio::Pin _enUsbBridgePin() {
        return GpioB[ 4 ];
    }

    static float dividerInversion( float top, float bottom ) {
        return ( top + bottom ) / bottom;
    }

    static Gpio::Pin _enChgPin() {
        return GpioC[ 15 ];
    }

    static Gpio::Pin _enBattToBusPin() {
        return GpioA[ 7 ];
    }

    static Gpio::Pin _enUsbToBusPin() {
        return GpioB[ 0 ];
    }
};