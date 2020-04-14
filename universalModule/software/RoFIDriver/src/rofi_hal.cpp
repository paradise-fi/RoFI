#include "rofi_hal.hpp"
#include <array>
#include <cassert>
#include <stdexcept>

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

    virtual void setVelocity( float velocity ) override {
        _poller.stop();
        _servo.rotate( Angle::rad( velocity ) );
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
            [this, callback, pos]() {
                auto currentPos = getPosition();
                if ( abs( currentPos - pos ) < Angle::deg( 2 * 0.325 ).rad() ) {
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
        throw std::runtime_error( "Herculex does not support torque setting" );
    }

    virtual void onError(
        std::function< void( Joint, Joint::Error ) > callback ) override
    {
        throw std::runtime_error( "Not implemented yet" );
    }
private:
    Capabilities _capabilities;
    rofi::herculex::Bus::Servo _servo;
    rtos::Timer _poller;
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
    cap.maxPosition = pi; // rad
    cap.minPosition = -pi; // rad
    cap.maxSpeed = 2 * pi; // rad / s
    cap.minSpeed = 2 * pi; // rad / s
    cap.maxTorque = 1.2; // N * m
    return cap;
}

rofi::herculex::Bus servoBus( UART_NUM_1, GPIO_NUM_4, GPIO_NUM_2 );
// Definition of all the joint drivers
std::array< std::shared_ptr< Joint::Implementation >, 3 > joints = {
    std::make_shared< JointLocal >( servoBus.getServo( 1 ), shoeJointCapability() ),
    std::make_shared< JointLocal >( servoBus.getServo( 2 ), shoeJointCapability() ),
    std::make_shared< JointLocal >( servoBus.getServo( 3 ), bodyJointCapability() )
};

// Definition of all the connector drivers
std::array< std::shared_ptr< Connector::Implementation >, 6 > connectors = {
    std::make_shared< ConnectorLocal >(),
    std::make_shared< ConnectorLocal >(),
    std::make_shared< ConnectorLocal >(),
    std::make_shared< ConnectorLocal >(),
    std::make_shared< ConnectorLocal >(),
    std::make_shared< ConnectorLocal >()
};

class RoFILocal : public RoFI::Implementation {
public:
    virtual RoFI::Id getId() const override {
        return 0;
    }

    virtual Joint getJoint( int index ) override {
        assert( index > 0 );
        assert( index < 3 );
        return joints[ index ];
    }

    virtual Connector getConnector( int index ) override {
        assert( index > 0 );
        assert( index < 6 );
        return connectors[ index ];
    };
};

// Definition of local RoFI driver
std::shared_ptr< RoFILocal > localRoFI;

} // namespace

namespace rofi::hal {

RoFI RoFI::getLocalRoFI() {
    return RoFI( localRoFI );
}

RoFI RoFI::getRemoteRoFI( RoFI::Id /*id*/ ) {
    throw std::runtime_error( "Remote calling is not implemented yet" );
}

} // namespace rofi::hal
