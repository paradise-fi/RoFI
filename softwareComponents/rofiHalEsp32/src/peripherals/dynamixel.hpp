#pragma once

#include <optional>
#include <mutex>
#include <cmath>
#include <string>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <atoms/units.hpp>

#include "dynamixel/packet.hpp"

namespace rofi::hal::dynamixel {

class ServoError : public std::runtime_error {
    using runtime_error::runtime_error;
};

inline int toPosition( Angle a ) {
    return a.rad() * 2048 / Angle::pi;
}

inline Angle toAngle( int rawPos ) {
    return Angle::rad( rawPos * Angle::pi / 2048.0 );
}

std::string errorName( Error e ) {
    // ESP32 uses GCC 8.4 so MagicEnum doesn't work, thus a hand-written
    // function is used
    switch( e ) {
        case Error::Nothing: return "Nothing";
        case Error::ResultFail: return "ResultFail";
        case Error::Instruction: return "Instruction";
        case Error::CRC: return "CRC";
        case Error::DataRange: return "DataRange";
        case Error::DataLength: return "DataLength";
        case Error::DataLimit: return "DataLimit";
        case Error::Access: return "Access";
        default: assert( false && "No such error" );
    }
}

std::string errorName( HardwareError e ) {
    // ESP32 uses GCC 8.4 so MagicEnum doesn't work, thus a hand-written
    // function is used
    std::string ret;
    auto add = [&]( const char* c ) {
        if ( !ret.empty() )
            ret += " | ";
        ret += c;
    };

    if ( e & HardwareError::InputVoltage )
        add( "InputVoltage" );
    if ( e & HardwareError::Overheating )
        add( "Overheating" );
    if ( e & HardwareError::MotorEncoder )
        add( "MotorEncoder" );
    if ( e & HardwareError::ElectricalShock )
        add( "ElectricalShock" );
    if ( e & HardwareError::Overloaded )
        add( "Overloaded");
    return ret;
}


/**
 * \brief Dynamixel servo bus
 *
 * Bus takes control of ESP32 UART peripheral and provides interface for sending
 * and receiving packets. On top of it it provides proxies in form of the Servo
 * class which can help you address individual servo motors.
 */
class Bus {
public:
    Bus( uart_port_t uart, gpio_num_t txrx, int baudrate, int defaultWaitTime = 20 / portTICK_PERIOD_MS )
        : _uart( uart ), _pin( txrx ), _defaultWaitTime( defaultWaitTime )
    {
        uart_config_t uart_config = {
            .baud_rate = baudrate,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .use_ref_tick = false
        };
        if ( uart_param_config( _uart, &uart_config ) != ESP_OK )
            throw ServoError( "Cannot initialize bus" );
        if ( uart_set_pin( _uart, _pin, UART_PIN_NO_CHANGE,
                            UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) != ESP_OK )
            throw ServoError( "Cannot assign pins to a bus" );
        if ( uart_driver_install( _uart, 256, 256, 0, nullptr, ESP_INTR_FLAG_IRAM ) != ESP_OK )
            throw ServoError( "Cannot install uart driver" );
    }

    void reconfigureBaudrate( int baudrate ) {
        if ( uart_set_baudrate( _uart, baudrate ) != ESP_OK )
            throw ServoError( "Cannot configure baudrate" );
    }

    /**
     * \brief Send a packet to the bus
     */
    void send( const Packet& p ) {
        switchToTx();
        uart_flush_input( _uart );
        uart_write_bytes( _uart, p.raw(), p.rawSize() );
    }

    /**
     * \brief Wait for finishing transmission
     */
    bool waitForTx( int timeout ) {
        auto res = uart_wait_tx_done( _uart, timeout );
        switch (res)
        {
        case ESP_ERR_TIMEOUT:
            return false;
        case ESP_FAIL:
            throw ServoError( "Couldn't switch to RX mode" );
        }
        return true;
    }

    /**
     * \brief Read a packet from the bus
     *
     * If not packet is received within the timout, std::nullopt is returned.
     * \param ticks timeout in FreeRTOS ticks
     */
    std::optional< Packet > read( int ticks = -1, bool flushInput = true ) {
        if ( ticks == -1 )
            ticks = _defaultWaitTime;
        if ( !switchToRx( ticks ) )
            return std::nullopt;
        if ( flushInput )
            uart_flush_input( _uart );
        PacketParser parser;
        uint8_t buff;
        int size = uart_read_bytes( _uart, &buff, 1, ticks );
        if ( size != 1 )
            return std::nullopt;
        while ( size == 1 && !parser.parseByte( buff ) ) {
            size = uart_read_bytes( _uart, &buff, 1 , ticks );
        }
        Packet p = parser.getPacket();
        if ( p.valid() )
            return p;
        return std::nullopt;
    }

    /**
     * \brief Proxy for controlling a single servomotor
     *
     * Servo can be safely copied and passed around. It provides methods to
     * directly control the servo without the need to construct packets
     * individually
     */
    class Servo {
    public:
        friend class Bus;
        /**
         * Reboot the servo motor. This is the only way to reset hardware error
         * status. The procedure blocks until the servo is available again.
         * After 20 tries, it raises exception.
         */
        void reboot() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::reboot( _id ) );
            auto response = _bus.read();
            validateResponse( response, "reboot" );

            // Wait for reboot completion
            for ( int i = 0; i != 20; i++ ) {
                _bus.send( Packet::ping( _id ) );
                auto response = _bus.read();
                if ( response.has_value() )
                    return;
            }
            throw ServoError( "Servo " + std::to_string( int( _id ) )
                            + " didn't wake up from reboot" );
        }

        /**
         * Get servo status
         */
        HardwareError status() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::read( _id, Register::HardwareErrorStatus ) );
            auto response = _bus.read();
            validateResponse( response, "reading status register" );
            return HardwareError( response->get< uint8_t >() );
        }

        void setPositionMode() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::OperatingMode, uint8_t( 3 ) ) );
            validateResponse( _bus.read(), "setting operating mode" );
        }

        void setExtendedPositionMode() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::OperatingMode, uint8_t( 4 ) ) );
            validateResponse( _bus.read(), "setting operating mode" );
        }

        void torqueOn() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::TorqueEnable, uint8_t( 1 ) ) );
            auto response = _bus.read();
            validateResponse( response, "setting torque on" );
        }

        void torqueOff() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::TorqueEnable, uint8_t( 0 ) ) );
            auto response = _bus.read();
            validateResponse( response, "setting torque off" );
        }

        /**
         * \brief Limit torque of the servomotor
         *
         * Range 0-1, limits by limitting PWM internally (so only an
         * approximation). Has to performed when torque is off.
         */
        void setTorqueLimit( float limit ) {
            assert( limit >= 0 && limit <= 1 );
            int16_t val = limit * 885;

            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::PWMLimit, val ) );
            auto response = _bus.read();
            validateResponse( response, "setting PWM limit" );
        }

        /**
         * \brief Get current position of the motor.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back
         */
        Angle getPosition() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::read( _id, Register::PresentPosition ) );
            auto response = _bus.read();
            validateResponse( response , "getting position" );
            return toAngle( response->get< int >() );
        }

        /**
         * \brief Get current speed of the motor in angle per second.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back
         */
        Angle getSpeed() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::read( _id, Register::PresentVelocity ) );
            auto response = _bus.read();
            validateResponse( response, "getting speed" );
            return toAngle( response->get< int >() );
        }

        /**
         * \brief Get current torque of the motor.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back. The torque is estimated based on PWM value.
         */
        float getTorque() {
            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::read( _id, Register::PresentLoad ) );
            auto response = _bus.read();
            validateResponse( response, "getting load" );
            float load = abs( response->get< int16_t >() ) / 10.0;
            return load;
        }

        /**
         * \brief Move the servo to given position
         */
        void move( Angle pos, Angle angleVelocity ) {
            assert( angleVelocity.rad() >= 0 && toPosition( angleVelocity ) < 32767 );

            std::scoped_lock lock( _bus._mutex );
            _bus.send( Packet::write( _id, Register::ProfileVelocity,
                                      toPosition( angleVelocity ) ) );
            validateResponse( _bus.read(), "setting profile speed" );
            _bus.send( Packet::write( _id, Register::GoalPosition,
                                      toPosition( pos ) ) );
            validateResponse( _bus.read(), "setting goal position" );
        }

        /**
         * \brief Start rotating the servo at given speed
         */
        void rotate( Angle velocity ) {
            std::scoped_lock lock( _bus._mutex );
            int rawVelocity = toPosition( velocity );
            _bus.send( Packet::write( _id, Register::ProfileVelocity,
                                      abs( rawVelocity ) ) );
            validateResponse( _bus.read(), "setting profile speed" );

            Register goal = rawVelocity < 0
                ? Register::MinPositionLimit
                : Register::MaxPositionLimit;
            _bus.send( Packet::read( _id, goal ) );
            auto response = _bus.read();
            validateResponse( response, "getting position limit" );

            _bus.send( Packet::write( _id, Register::GoalPosition,
                                      response->get< int >() ) );
            validateResponse( response, "setting goal position" );
        }

    private:
        Servo( Id id, Bus& bus ) : _id( id ), _bus( bus ) {}

        void validateResponse( const std::optional< Packet >& packet,
                               const std::string& requestName )
        {
            if ( !packet.has_value() )
                throw ServoError( "Servo " + std::to_string( int( _id ) )
                     + " didn't respond to " + requestName );
            auto error = packet->error();
            if ( error != Error::Nothing )
                throw ServoError( "Servo " + std::to_string( int ( _id ) )
                     + " returned error " + errorName( error ) );
        }

        Id _id;
        Bus& _bus;
    };

    Servo getServo( Id id ) {
        return Servo( id, *this );
    }

    Servo allServos() {
        return Servo( 0xFE, *this );
    }
private:
    /**
     * /brief Switch to TX mode of the half-duplex UART.
     */
    void switchToTx() {
        ESP_ERROR_CHECK( uart_set_pin( _uart, _pin, UART_PIN_NO_CHANGE,
                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) );
    }

    /**
     * /brief Switch to RX mode of the half-duplex UART. Return true if
     * successful
     */
    bool switchToRx( int timeout ) {
        if ( !waitForTx( timeout ) )
            return false;

        ESP_ERROR_CHECK( uart_set_pin( _uart, UART_PIN_NO_CHANGE, _pin,
                      UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) );
        return true;
    }

    uart_port_t _uart;
    gpio_num_t  _pin;
    int         _defaultWaitTime;
    std::mutex  _mutex;
};

} // rofi::hal::dynamixel