#include <cassert>
#include <future>
#include <iostream>

#include "rofi_hal.hpp"

constexpr int connectorA = 1;
constexpr int connectorB = 4;

constexpr bool useSetPosition = true;


template < typename Callback >
void setToLimitPos( rofi::hal::Joint joint, bool max, Callback && callback )
{
    using rofi::hal::Joint;

    double speed = joint.maxSpeed() / 4;

    if constexpr ( useSetPosition )
    {
        double position = max ? joint.maxPosition() : joint.minPosition();
        assert( std::abs( position ) < 10.0 );
        joint.setPosition(
                position,
                speed,
                [ callback = std::forward< Callback >( callback ) ]( Joint ) { callback(); } );
    }
    else
    {
        joint.setVelocity( ( max ? 1 : -1 ) * speed );
        rofi::hal::RoFI::wait( 4000, std::forward< Callback >( callback ) );
    }
}

void checkConnected( rofi::hal::Connector connector )
{
    using namespace rofi::hal;

    RoFI::wait( 1000, [ connectorConst = std::move( connector ) ] {
        Connector connector = connectorConst;
        auto state = connector.getState();
        if ( state.position != ConnectorPosition::Extended || state.connected )
        {
            // Already retracted or successfully connected
            return;
        }

        connector.disconnect();
        RoFI::wait( 500, [ connectorConst = std::move( connector ) ] {
            Connector connector = connectorConst;
            connector.connect();

            checkConnected( connector );
        } );
    } );
}

void oneMove( int connector )
{
    using namespace rofi::hal;
    assert( connector == connectorA || connector == connectorB );

    int otherConnector = connector == connectorA ? connectorB : connectorA;

    RoFI localRofi = RoFI::getLocalRoFI();
    RoFI remoteRofi = RoFI::getRemoteRoFI( 1 );

    // Disconnect old connections
    std::cout << "Disconnect old\n";
    localRofi.getConnector( otherConnector ).disconnect();
    remoteRofi.getConnector( otherConnector ).disconnect();

    // Wait 1 second
    {
        std::cout << "Wait for 1s\n";
        std::promise< void > waitPromise;
        auto waitFuture = waitPromise.get_future();

        RoFI::wait( 1000, [ &waitPromise ] { waitPromise.set_value(); } );
        waitFuture.get();
    }

    // Move joints
    {
        std::cout << "Move joints\n";
        std::promise< void > localPromise1, localPromise2, remotePromise1, remotePromise2;
        auto localFuture1 = localPromise1.get_future();
        auto localFuture2 = localPromise2.get_future();
        auto remoteFuture1 = remotePromise1.get_future();
        auto remoteFuture2 = remotePromise2.get_future();

        bool toMax = connector == connectorA;

        setToLimitPos( localRofi.getJoint( 1 ), toMax, [ &localPromise1 ] {
            localPromise1.set_value();
        } );
        setToLimitPos( localRofi.getJoint( 2 ), toMax, [ &localPromise2 ] {
            localPromise2.set_value();
        } );
        setToLimitPos( remoteRofi.getJoint( 1 ), toMax, [ &remotePromise1 ] {
            remotePromise1.set_value();
        } );
        setToLimitPos( remoteRofi.getJoint( 2 ), toMax, [ &remotePromise2 ] {
            remotePromise2.set_value();
        } );

        localFuture1.get();
        localFuture2.get();
        remoteFuture1.get();
        remoteFuture2.get();
    }

    // Connect new connections
    std::cout << "Connect new\n";
    localRofi.getConnector( otherConnector ).connect();
    remoteRofi.getConnector( otherConnector ).connect();

    std::cout << "Check connection\n";
    checkConnected( localRofi.getConnector( otherConnector ) );
}

int main()
{
    using namespace rofi::hal;

    std::cout << "Starting basic move example\n";

    RoFI localRofi = RoFI::getLocalRoFI();
    RoFI remoteRofi = RoFI::getRemoteRoFI( 2 );

    assert( localRofi.getId() != 2 );

    assert( localRofi.getDescriptor().connectorCount > connectorA );
    assert( localRofi.getDescriptor().connectorCount > connectorB );
    assert( remoteRofi.getDescriptor().connectorCount > connectorA );
    assert( remoteRofi.getDescriptor().connectorCount > connectorB );

    localRofi.getConnector( connectorA ).onConnectorEvent( []( Connector, ConnectorEvent event ) {
        if ( event == ConnectorEvent::Connected )
        {
            oneMove( connectorA );
        }
    } );
    localRofi.getConnector( connectorB ).onConnectorEvent( []( Connector, ConnectorEvent event ) {
        if ( event == ConnectorEvent::Connected )
        {
            oneMove( connectorB );
        }
    } );

    localRofi.getConnector( connectorA ).connect();
    remoteRofi.getConnector( connectorA ).connect();

    while ( true )
    {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }
}
