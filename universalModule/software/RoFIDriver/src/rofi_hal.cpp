#include "rofi_hal.hpp"
#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>

#include <peripherals/herculex.hpp>
#include <freeRTOS.hpp>

namespace {

using namespace rofi::hal;

/**
 * HAL Connector driver implementation for RoFICoM 2.5 on ESP32
 */
class ConnectorLocal : public Connector::Implementation {
public:
    virtual ConnectorState getState() const override {
        return {};
    }

    virtual void connect() override {
        // ToDo: Implement
    }

    virtual void disconnect() override {
        // ToDo: Implement
    }

    virtual void onConnectorEvent(
        std::function< void ( Connector, ConnectorEvent ) > callback )
    {
        // ToDo: Implement
    }

    virtual void onPacket(
        std::function< void( Connector, Packet ) > callback ) override
    {
        // ToDo: Implement
    }

    virtual void send( Packet packet ) override {
        // ToDo: Implement
    }

    virtual void connectPower( ConnectorLine ) override {
        // ToDo: Implement
    }

    virtual void disconnectPower( ConnectorLine ) override {
        // ToDo: Implement
    }
};

/**
 * HAL Joint driver implementation for Herculex DRS-0101 Servomotors
 */
class JointLocal :
    public Joint::Implementation,
    public std::enable_shared_from_this< JointLocal >
{
public:
    JointLocal( rofi::herculex::Bus::Servo servo, const Capabilities& cap ):
        _capabilities( cap ), _servo( std::move( servo ) )
    {
        _servo.resetErrors();
        _servo.torqueOn();
        _servo.setLimits( Angle::rad( cap.minPosition ),
                          Angle::rad( cap.maxPosition ) );
    }

    virtual const Capabilities& getCapabilities() override {
        return _capabilities;
    }

    virtual float getVelocity() override {
        return _servo.getSpeed().rad();
    }

    /**
     * \brief set velocity of the joint
     *
     * Unfortunately, Herculex does not support velocity control, let's fake it.
     * We also cannot do proper regulator loop, so fake it by PWM and emulating
     * rotational limits by a poller.
     */
    virtual void setVelocity( float velocity ) override {
        using namespace std::chrono_literals;

        _poller = rtos::Timer();
        float maxVelocity = 2 * pi;
        velocity = std::min( maxVelocity, std::max( -maxVelocity, velocity ) );
        int pwm = velocity / maxVelocity * 1023;
        _servo.rotate( pwm );
        _poller = rtos::Timer( 20ms, rtos::Timer::Type::Periodic,
            [this, pwm]() {
                auto pos = getPosition();
                if ( pwm < 0 &&
                     pos < _capabilities.minPosition + Angle::deg( 10 * 0.325 ).rad() )
                {
                    this->_servo.move( Angle::rad( _capabilities.minPosition ), 100ms );
                }
                else if ( pwm > 0 &&
                          pos > _capabilities.maxPosition - Angle::deg( 10 * 0.325 ).rad() )
                {
                    this->_servo.move( Angle::rad( _capabilities.maxPosition ), 100ms );
                }
                else
                    this->_servo.rotate( pwm );
            } );
        _poller.start();
    }

    virtual float getPosition() override {
        return _servo.getPosition().rad();
    }

    virtual void setPosition( float pos, float velocity,
        std::function< void( Joint ) > callback ) override
    {
        using namespace std::chrono_literals;

        auto currentPos = getPosition();
        auto distance = abs( pos - currentPos );
        auto duration = std::chrono::milliseconds( int( distance / velocity * 1000 ) );
        _poller = rtos::Timer( 20ms, rtos::Timer::Type::Periodic,
            [ this, callback, pos ]() {
                auto currentPos = getPosition();
                if ( abs( currentPos - pos ) < Angle::deg( 4 * 0.325 ).rad() ) {
                    callback( rofi::hal::Joint( this->shared_from_this() ) );
                    _poller.stop();
                }
            } );
        _servo.move( Angle::rad( pos ), duration );
        _poller.start();
    }

    virtual float getTorque() override {
        return _servo.getTorque() * _capabilities.maxTorque;
    }

    virtual void setTorque( float torque ) override {
        torque = std::min( _capabilities.maxTorque, std::max( -_capabilities.maxTorque, torque ) );
        int pwm = torque / _capabilities.maxTorque * 1023;
        _servo.rotate( pwm );
    }

    virtual void onError(
        std::function< void( Joint, Joint::Error, const std::string& msg ) > callback ) override
    {
        using namespace std::chrono_literals;
        _errorCallback = callback;
        _errorPoller = rtos::Timer( 200ms, rtos::Timer::Type::Periodic,
            [ this ]() {
                auto flags = _servo.status();
                if ( !rofi::herculex::containsError( flags ) )
                    return;
                _errorCallback(
                    rofi::hal::Joint( this->shared_from_this() ),
                    rofi::hal::Joint::Error::Hardware,
                    rofi::herculex::errorMessage( flags ) );
                _servo.resetErrors();
            } );
        _errorPoller.start();
    }
private:
    Capabilities _capabilities;
    rofi::herculex::Bus::Servo _servo;
    rtos::Timer _poller;
    rtos::Timer _errorPoller;
    std::function< void( Joint, Joint::Error, const std::string& msg ) > _errorCallback;
};

rofi::hal::Joint::Implementation::Capabilities shoeJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = pi / 2; // rad
    cap.minPosition = -pi / 2; // rad
    cap.maxSpeed = 2 * pi; // rad / s
    cap.minSpeed = 2 * pi; // rad / s
    cap.maxTorque = 1.2; // N * m
    return cap;
}

rofi::hal::Joint::Implementation::Capabilities bodyJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = 0.9 * pi; // rad
    cap.minPosition = -0.9 * pi; // rad
    cap.maxSpeed = 2 * pi; // rad / s
    cap.minSpeed = 2 * pi; // rad / s
    cap.maxTorque = 1.2; // N * m
    return cap;
}

class RoFILocal : public RoFI::Implementation {
public:
    RoFILocal():
        _servoBus( UART_NUM_1, GPIO_NUM_4, GPIO_NUM_2 ),
        _joints( {
            std::make_shared< JointLocal >( _servoBus.getServo( 1 ), shoeJointCapability() ),
            std::make_shared< JointLocal >( _servoBus.getServo( 2 ), shoeJointCapability() ),
            std::make_shared< JointLocal >( _servoBus.getServo( 3 ), bodyJointCapability() )
        } ),
        _connectors( {
            std::make_shared< ConnectorLocal >(),
            std::make_shared< ConnectorLocal >(),
            std::make_shared< ConnectorLocal >(),
            std::make_shared< ConnectorLocal >(),
            std::make_shared< ConnectorLocal >(),
            std::make_shared< ConnectorLocal >()
        } )
    { }

    virtual RoFI::Id getId() const override {
        return 0;
    }

    virtual Joint getJoint( int index ) override {
        assert( index >= 0 );
        assert( index < 3 );
        return Joint( _joints[ index ] );
    }

    virtual Connector getConnector( int index ) override {
        assert( index >= 0 );
        assert( index < 6 );
        return Connector( _connectors[ index ] );
    };

private:
    rofi::herculex::Bus _servoBus;
    std::array< std::shared_ptr< Joint::Implementation >, 3 > _joints;
    std::array< std::shared_ptr< Connector::Implementation >, 6 > _connectors;
};

} // namespace

namespace rofi::hal {

RoFI RoFI::getLocalRoFI() {
    // Definition of local RoFI driver
    static std::shared_ptr< RoFILocal > localRoFI = std::make_shared< RoFILocal >();
    return RoFI( localRoFI );
}

RoFI RoFI::getRemoteRoFI( RoFI::Id /*id*/ ) {
    throw std::runtime_error( "Remote calling is not implemented yet" );
}

} // namespace rofi::hal
