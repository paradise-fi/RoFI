#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "initial.hpp"
#include "fizzbuzz.hpp"

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

    int id = RoFI::getLocalRoFI().getId();
    std::cout << "This module is: " << id << "\n";
    NetworkManager net( RoFI::getLocalRoFI() );
    Ip6Addr addr = createAddress( id );
    net.addAddress( addr, 80, net.interface( "rl0" ) );
    net.setUp();
    
    auto rtProto = net.addProtocol( rofi::net::SimpleReactive() );
    net.setProtocol( *rtProto );

    auto messageDistributorProtocol = net.addProtocol( rofi::net::MessageDistributor( addr, net ) );
    net.setProtocol( *messageDistributorProtocol );
    
    std::unique_ptr< ElectionProtocolBase > election = std::make_unique< LRElect >( net, *reinterpret_cast< MessageDistributor* >( messageDistributorProtocol ), addr, 1, 3 );

    MessageDistributor* messageDistributor = reinterpret_cast< MessageDistributor* >( messageDistributorProtocol );

    // Instantiate the Distribution Manager
    DistributedTaskManager manager(
        std::move( election ), addr,
        *messageDistributor, std::move( pcb ) );

    // Register the distributed functions.
    manager.functions().registerFunction< int >( InitialFunction( id, manager ));
    manager.functions().registerFunction< int, int >( FizzBuzz( id, manager ) );

    // Start the Distribution Manager -> Ensures the used election algorithm is running.
    manager.start();

    while ( true ){
        // A single 'tick' of the manager instance.
        manager.doWork();
    }
}

int main( void ) {
    distributionManagerFizzBuzz( );
    return 0;
}