#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "exampleLogger.hpp"
#include "initial.hpp"
#include "disconnect.hpp"

#include "botState.hpp"
#include "configProtocol.hpp"
#include "move.hpp"

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

void distributionManagerFizzBuzz() {
    std::cout << "Starting Complex RoFI Distribution Manager example\n";
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
    
    std::unique_ptr< ElectionProtocolBase > election = std::make_unique< LRElect >( net, *reinterpret_cast< MessageDistributor* >( messageDistributor ), addr, 1, 3 );
    

    // Instantiate the Distribution Manager
    DistributedTaskManager manager(
        std::move( election ), addr,
        *reinterpret_cast< MessageDistributor* >( messageDistributor ), std::move( pcb ) );
    
    manager.useLogger( ExampleLogger() );

    std::set< Ip6Addr > requesters;

    manager.callbacks().registerTaskRequestCallback([&requesters]( DistributedTaskManager&, const Ip6Addr& requester ) {
        if ( requesters.find( requester ) == requesters.end() )
        {
            requesters.emplace( requester );
        }
        return false;
    });

    // Register the distributed functions.
    manager.functions().registerFunction< ModuleState >( InitialFunction( manager, botState, requesters ) );
    manager.functions().registerFunction< int, int >( Disconnect( manager, botState ) );
    manager.functions().registerFunction< MoveResult, int, float, float >( Move( manager, botState ) ); 

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