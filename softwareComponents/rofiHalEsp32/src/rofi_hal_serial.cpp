#include <rofi_hal.hpp>

#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <variant>
#include <thread>
#include <atomic>
#include <cmath>
#include <type_traits>

#include <freeRTOS.hpp>

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include <espDriver/gpio.hpp>
#include <espDriver/spi.hpp>

#include <bsp.hpp>
#include <atoms/util.hpp>
#include <logging.hpp>

namespace {

using namespace rofi::hal;
using namespace std::string_literals;

/**
 * \brief HAL Connector driver implementation for RoFICoM 2.5 on ESP32
 */
class ConnectorLocal:
    public Connector::Implementation,
    public std::enable_shared_from_this< ConnectorLocal >
{
public:
    virtual rofi::hal::ConnectorState getState() override {
        // TBA
    }

    virtual void connect() override {}

    virtual void disconnect() override {}

    virtual void onConnectorEvent(
        std::function< void ( Connector, ConnectorEvent ) > callback )
    {
    }

    virtual void onPacket(
        std::function< void( Connector, uint16_t, PBuf ) > callback ) override
    {
        _packetCallback = callback;
    }

    virtual void send( uint16_t contentType, PBuf packet ) override {
        // TBA
    }

    virtual void connectPower( ConnectorLine line ) override {}

    virtual void disconnectPower( ConnectorLine line ) override {}

    ConnectorLocal( /* TBA: SoftwareSerial handle *// )
        : _bus( bus ), _cs( cs ), _receiveCmdCounter( 0 ), _interruptCounter( 0 )
    {
        _setupCs();
    }
private:
    std::function< void( Connector, uint16_t, PBuf ) > _packetCallback;
    // TBA Software serial
};

/**
 * HAL Joint driver implementation based on Servomotor abstraction
 */
class JointLocal :
    public Joint::Implementation,
    public std::enable_shared_from_this< JointLocal >
{
public:
    JointLocal(const Capabilities& cap ):
        _capabilities( cap )
    { }

    virtual const Capabilities& getCapabilities() override {
        return _capabilities;
    }

    virtual float getVelocity() override {
        return 0;
    }

    virtual void setVelocity( float velocity ) override { }

    virtual float getPosition() override {
        return 0;
    }

    virtual void setPosition( float pos, float velocity,
        std::function< void( Joint ) > callback ) override
    {}

    virtual float getTorque() override {
        return 0;
    }

    virtual void setTorque( float torque ) override { }

    virtual void onError(
        std::function< void( Joint, Joint::Error, const std::string& msg ) > callback ) override
    {}
private:
    Capabilities _capabilities;
};

rofi::hal::Joint::Implementation::Capabilities shoeJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = Angle::pi / 2 * 1.1;   // rad
    cap.minPosition = -Angle::pi / 2 * 1.1;  // rad
    cap.maxSpeed    = 2 * Angle::pi;         // rad / s
    cap.minSpeed    = 2 * Angle::pi;         // rad / s
    cap.maxTorque   = 1.2;                   // N * m
    return cap;
}

rofi::hal::Joint::Implementation::Capabilities bodyJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = 0.9 * Angle::pi;   // rad
    cap.minPosition = -0.9 * Angle::pi;  // rad
    cap.maxSpeed    = 2 * Angle::pi;     // rad / s
    cap.minSpeed    = 2 * Angle::pi;     // rad / s
    cap.maxTorque   = 1.2;               // N * m
    return cap;
}

class RoFILocal : public RoFI::Implementation {
public:
    RoFILocal() try:
        _joints( {
            std::make_shared< JointLocal >( shoeJointCapability() ),
            std::make_shared< JointLocal >( shoeJointCapability() ),
            std::make_shared< JointLocal >(  bodyJointCapability() )
        } ),
        _connectorBus( HSPI_HOST, GPIO_NUM_19, GPIO_NUM_18, 100000 ),
        _connectors( {
            std::make_shared< ConnectorLocal >( /* TBA */ ),
            std::make_shared< ConnectorLocal >( /* TBA */ ),
            std::make_shared< ConnectorLocal >( /* TBA */ ),
            std::make_shared< ConnectorLocal >( /* TBA */ ),
            std::make_shared< ConnectorLocal >( /* TBA */ ),
            std::make_shared< ConnectorLocal >( /* TBA */ )
        } )
    {} catch ( const std::runtime_error& e ) {
        throw std::runtime_error( "Cannot intialize local RoFI Driver: "s + e.what() );
    }

    virtual RoFI::Id getId() const override {
        // TBA
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

    virtual RoFI::Descriptor getDescriptor() const override {
        return { 3, 6 };
    };

private:
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
