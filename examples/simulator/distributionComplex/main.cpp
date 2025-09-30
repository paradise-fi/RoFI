#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "initial.hpp"
#include "disconnect.hpp"

#include "botState.hpp"
#include "configProtocol.hpp"

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

Ip6Addr createAddress( int id ) {
    std::stringstream ss;
    ss << "fc07:0:0:";
    ss << id;
    ss << "::1";

    return Ip6Addr( ss.str() );
}

/// In this simple example, you will learn how to create a simple program using
/// the distribution manager. In this example, follower nodes will generate values
/// that will be sent to the leader. The leader will take these values and play the fizzbuzz
/// game with them.
void distributionManagerFizzBuzz() {
    std::cout << "Starting simple RoFI Distribution Manager FizzBuzz example\n";
    tcpip_init( nullptr, nullptr );

    LOCK_TCPIP_CORE();
    auto pcb = std::make_unique< udp_pcb >( *udp_new() );
    UNLOCK_TCPIP_CORE();

    auto rofi = RoFI::getLocalRoFI();
    int id = rofi.getId();
    std::cout << "This module is: " << id << "\n";
    NetworkManager net( RoFI::getLocalRoFI() );
    Ip6Addr addr = createAddress( id );
    net.addAddress( addr, 80, net.interface( "rl0" ) );
    net.setUp();

    BotState botState( addr );
    botState.modules[ addr ] = ModuleState(addr);

    auto descriptor = rofi.getDescriptor();
    for ( int cidx = 0; cidx < descriptor.connectorCount; ++cidx )
    {
        botState.modules[ addr ].connectors.push_back( ConnectorStatus( cidx, std::nullopt, std::nullopt ) );
    }

    for ( int jidx = 0; jidx < descriptor.jointCount; ++jidx )
    {
        auto joint = rofi.getJoint( jidx );
        botState.modules[ addr ].joints.push_back( JointStatus( joint.getPosition(), joint.maxPosition(), joint.minPosition() ) );
    }
    
    // A network manager protocol that gathers data about the neighbours and creates the module state object from it.
    auto cfgProto = net.addProtocol( rofi::net::ConfigurationProtocol(botState));
    net.setProtocol( *cfgProto );
    
    auto rtProto = net.addProtocol( rofi::net::SimpleReactive() );
    net.setProtocol( *rtProto );
    
    auto messageDistributor = net.addProtocol( rofi::net::MessageDistributor( addr, net ) );
    net.setProtocol( *messageDistributor );
    
    std::unique_ptr< ElectionProtocolBase > election = std::make_unique< LRElect >( net, reinterpret_cast< MessageDistributor* >( messageDistributor ), addr );
    

    // Instantiate the Distribution Manager
    DistributedTaskManager manager(
        std::move( election ), addr,
        reinterpret_cast< MessageDistributor* >( messageDistributor ), std::move( pcb ) );
    
    std::set< Ip6Addr > requesters;

    manager.registerTaskRequestCallback([&requesters]( DistributedTaskManager&, Ip6Addr& requester ) {
        std::cout << "task request cb from " << requester << std::endl;
        if ( requesters.find( requester ) == requesters.end() )
        {
            requesters.emplace( requester );
        }
        return false;
    });

    // Create distributed function instances.
    std::unique_ptr< DistributedFunction< ModuleState > > initial = std::make_unique< Initial >( manager, botState, requesters );
    std::unique_ptr< DistributedFunction< int, int > > disconnect = std::make_unique< Disconnect >( manager, botState );

    int initialFunctionId = initial->functionId();

    // Register the distributed functions.
    manager.functionRegistry().registerFunction< ModuleState >( std::move( initial ) );
    manager.functionRegistry().registerFunction< int, int >( std::move( disconnect ) );

    // Register the ID of the initial task
    manager.functionRegistry().setInitialTask( initialFunctionId );

    // Start the Distribution Manager -> Ensures the used election algorithm is running.
    manager.start( id );

    while ( true ){
        sleep( 1 );
        // A single 'tick' of the manager instance.
        manager.doWork();
    }
}

int main( void ) {
    distributionManagerFizzBuzz( );
    return 0;
}