#include <future>
#include <iostream>

#include <atoms/units.hpp>

#include <rofi_hal.hpp>


namespace hal = rofi::hal;
using namespace std::chrono_literals;

void blockingMove( hal::Joint joint, Angle pos, float speedMultiplier = 0.8f )
{
    auto moveEndPromise = std::promise< void >();

    auto speed = joint.maxSpeed() * speedMultiplier;
    joint.setPosition( pos.rad(), speed, [ & ]( hal::Joint ) { moveEndPromise.set_value(); } );

    moveEndPromise.get_future().get();
}

// Expects that there is a roficom that can be connected to
void blockingConnect( hal::Connector connector )
{
    if ( auto state = connector.getState(); state.position == hal::ConnectorPosition::Extended ) {
        throw std::runtime_error( "connector already extended" );
    }
    auto connectionPromise = std::promise< void >();

    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( hal::Connector, hal::ConnectorEvent event ) {
        if ( event == hal::ConnectorEvent::Connected ) {
            std::call_once( onceFlag, [ & ]() { connectionPromise.set_value(); } );
        }
    } );

    connector.connect();

    connectionPromise.get_future().get();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );
}

void blockingDisconnect( hal::Connector connector )
{
    if ( auto state = connector.getState(); !state.connected ) {
        throw std::runtime_error( "connector not connected to another module" );
    }
    auto connectionPromise = std::promise< void >();

    auto onceFlag = std::once_flag{};
    connector.onConnectorEvent( [ & ]( hal::Connector, hal::ConnectorEvent event ) {
        if ( event == hal::ConnectorEvent::Disconnected ) {
            std::call_once( onceFlag, [ & ]() { connectionPromise.set_value(); } );
        }
    } );

    connector.disconnect();

    connectionPromise.get_future().get();
    // Clear callback to avoid dangling references in lambda
    connector.onConnectorEvent( nullptr );
}

int main()
{
    auto localRoFI = hal::RoFI::getLocalRoFI();
    std::cout << "Hello world from module " << localRoFI.getId() << "!\n";

    auto connectorA = localRoFI.getConnector( 2 ); // A-Z
    auto connectorB = localRoFI.getConnector( 5 ); // B-Z

    auto jointA = localRoFI.getJoint( 0 ); // alpha
    auto jointB = localRoFI.getJoint( 1 ); // beta

    connectorB.disconnect(); // Default for roficom position is extended

    for ( int i = 0; i < 2; i++ ) {
        blockingMove( jointA, 90_deg );
        blockingMove( jointB, 90_deg );

        blockingConnect( connectorB );
        blockingDisconnect( connectorA );

        blockingMove( jointA, -90_deg );
        blockingMove( jointB, -90_deg );

        blockingConnect( connectorA );
        blockingDisconnect( connectorB );
    }

    blockingMove( jointA, 90_deg );
    blockingMove( jointB, 90_deg );

    blockingConnect( connectorB );
    blockingDisconnect( connectorA );

    blockingMove( jointA, 0_deg );
    blockingMove( jointB, 0_deg );
}
