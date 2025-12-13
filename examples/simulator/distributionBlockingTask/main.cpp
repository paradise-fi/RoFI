#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "systemMethods/exampleLogger.hpp"
#include "initial.hpp"
#include "blocking.hpp"
#include "nonBlocking.hpp"
#include "systemMethods/naiveBarrier.hpp"

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
/// This example serves to show the semantics of blocking tasks and barrier tasks.
void distributionManagerFizzBuzz() {
    std::cout << "Starting RoFI Distribution Manager example showing off blocking task semantics\n";
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
        
    bool terminate = false;
    bool nonBlockingCalledFirst = false;


    // Register the distributed functions.
    manager.functions().registerFunction< int >( InitialFunction( id, manager ) );
    manager.functions().registerFunction< int, int >( BlockingFunction( id, manager, nonBlockingCalledFirst ) );
    manager.functions().registerFunction< int, int >( NonBlockingFunction( id, manager, nonBlockingCalledFirst ) );
    if ( !manager.functions().registerFunction< Ip6Addr >( NaiveBarrier( addr, manager ) ) )
    {
        std::cout << "Barrier failed to register." << std::endl;
    }

    // Register logger implementation
    manager.loggingService().useLogger( ExampleLogger(), LogVerbosity::High );

    // Start the Distribution Manager -> Ensures the used election algorithm is running.
    manager.start( id );

    while ( !terminate ) {
        // A single 'tick' of the manager instance.
        manager.doWork();
    }
}

int main( void ) {
    distributionManagerFizzBuzz( );
    return 0;
}