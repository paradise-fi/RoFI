#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "implementation/replicatedMemory.hpp"
#include "exampleLogger.hpp"
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

/// In this simple example, you will learn how to use distributed memory within the memory manager. 
/// In this example, follower nodes will generate values that will be stored in memory. 
///The leader will take these values and play the fizzbuzz game with them.
void distributionManagerFizzBuzz() {
    std::cout << "Starting RoFI Distribution Manager FizzBuzz example with memory\n";
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

    auto messageDistributor = net.addProtocol( rofi::net::MessageDistributor( addr, net ) );
    net.setProtocol( *messageDistributor );
    
    std::unique_ptr< ElectionProtocolBase > election = std::make_unique< LRElect >( net, reinterpret_cast< MessageDistributor* >( messageDistributor ), addr );

    // Instantiate the Distribution Manager
    DistributedTaskManager manager(
        std::move( election ), addr,
        reinterpret_cast< MessageDistributor* >( messageDistributor ), std::move( pcb ) );

    // Register the distributed functions.
    manager.registerFunction< int >( InitialFunction( id, manager ) );
    manager.registerFunction< int, int >( FizzBuzz( id, manager ) );

    // Register the memory implementation - the memory implementation is responsible for 
    // initiating memory-relevant communication, hence why the sender is passed too.
    manager.memoryService().useMemory( 
        std::make_unique< ReplicatedMemory >() );

    // Register logger implementation
    manager.useLogger( ExampleLogger() );

    // Start the Distribution Manager -> Ensures the used election algorithm is running.
    manager.start( id );

    while ( true ){
        // A single 'tick' of the manager instance.
        manager.doWork();
    }
}

int main( void ) {
    distributionManagerFizzBuzz( );
    return 0;
}