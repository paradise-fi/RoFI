#include <cassert>
#include <future>
#include <iostream>
#include <thread>

#include "rofi_hal.hpp"


using namespace rofi::hal;

constexpr RoFI::Id remoteRofiId = 2;

constexpr int connectorA = 2;
constexpr int connectorB = 5;

constexpr bool useSetPosition = true;


template < typename Callback >
void setToLimitPos( Joint joint, bool max, Callback && callback )
{
    auto speed = joint.maxSpeed() / 4;

    if constexpr ( useSetPosition )
    {
        auto position = max ? joint.maxPosition() : joint.minPosition();
        assert( std::abs( position ) < 10.0f );
        joint.setPosition(
                position,
                speed,
                [ callback = std::forward< Callback >( callback ) ]( Joint ) { callback(); } );
    }
    else
    {
        joint.setVelocity( ( max ? 1 : -1 ) * speed );
        RoFI::postpone( 4000, std::forward< Callback >( callback ) );
    }
}

void waitConnected( Connector connector )
{
    while ( true )
    {
        RoFI::sleep( 2000 );
        auto state = connector.getState();
        if ( state.position != ConnectorPosition::Extended )
        {
            return;
        }
        if ( state.connected )
        {
            return;
        }

        std::cout << "Retracting\n";
        connector.disconnect();
        RoFI::sleep( 2000 );

        std::cout << "Extending\n";
        connector.connect();
    }
}

void oneMove( int connector )
{
    assert( connector == connectorA || connector == connectorB );

    int newConnector = connector == connectorA ? connectorB : connectorA;

    RoFI localRofi = RoFI::getLocalRoFI();
    RoFI remoteRofi = RoFI::getRemoteRoFI( remoteRofiId );

    // Make sure the target connector is free. Keep the current connector attached
    // while moving so it constrains the modules into the next pose.
    std::cout << "Disconnect target\n";
    localRofi.getConnector( newConnector ).disconnect();
    remoteRofi.getConnector( newConnector ).disconnect();

    // Wait 1 second
    std::cout << "Wait for 1s\n";
    RoFI::sleep( 1000 );

    // Move joints
    {
        std::cout << "Move joints\n";
        std::promise< void > localPromise1, localPromise2, remotePromise1, remotePromise2;
        auto localFuture1 = localPromise1.get_future();
        auto localFuture2 = localPromise2.get_future();
        auto remoteFuture1 = remotePromise1.get_future();
        auto remoteFuture2 = remotePromise2.get_future();

        bool toMax = connector == connectorA;

        setToLimitPos( localRofi.getJoint( 0 ), toMax, [ &localPromise1 ] {
            localPromise1.set_value();
        } );
        setToLimitPos( localRofi.getJoint( 1 ), toMax, [ &localPromise2 ] {
            localPromise2.set_value();
        } );
        setToLimitPos( remoteRofi.getJoint( 0 ), toMax, [ &remotePromise1 ] {
            remotePromise1.set_value();
        } );
        setToLimitPos( remoteRofi.getJoint( 1 ), toMax, [ &remotePromise2 ] {
            remotePromise2.set_value();
        } );

        localFuture1.get();
        localFuture2.get();
        remoteFuture1.get();
        remoteFuture2.get();
    }

    // Connect new connections
    std::cout << "Connect new\n";
    localRofi.getConnector( newConnector ).connect();
    remoteRofi.getConnector( newConnector ).connect();

    std::cout << "Check connection\n";
    waitConnected( localRofi.getConnector( newConnector ) );
}

int main()
{
    std::cout << "Starting basic move example\n";

    RoFI localRofi = RoFI::getLocalRoFI();
    RoFI remoteRofi = RoFI::getRemoteRoFI( remoteRofiId );

    assert( localRofi.getId() != remoteRofiId );

    assert( localRofi.getDescriptor().connectorCount > connectorA );
    assert( localRofi.getDescriptor().connectorCount > connectorB );
    assert( remoteRofi.getDescriptor().connectorCount > connectorA );
    assert( remoteRofi.getDescriptor().connectorCount > connectorB );

    int connector = connectorA;
    while ( true )
    {
        oneMove( connector );
        connector = connector == connectorA ? connectorB : connectorA;
    }
}
