#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "testMemory.hpp"
#include "systemMethods/exampleLogger.hpp"
#include "initial.hpp"
#include "read.hpp"
#include "sendSave.hpp"
#include "check.hpp"
#include "address.cpp"
#include "message.hpp"

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

/// In this simple example, you will learn how to use distributed memory with blocking reading within the memory manager. 
void distributionManagerBlockingMemory() {
    std::cout << "Starting RoFI Distribution Manager FizzBuzz example with distributed memory and blocking distributed reads\n";
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

    manager.callbacks().registerBlockingCustomMessageCallback( 
        []( DistributedTaskManager& mgr, const Ip6Addr&, uint8_t* dataBuffer, unsigned int bufferSize )
        {
            std::cout << "Blocking message custom callback" << std::endl;
            MessagingResult result( false, sizeof( int ) );
            if ( bufferSize > 0 )
            {
                result.success = true;
                int data = as< int >( dataBuffer );
                mgr.memory().saveData< Message >( Message( std::string( "Stored Message" ) ), data );
                std::memcpy( result.rawData.data(), &data, result.rawData.size() );
            }
            return result;
        } );

    // Register the distributed functions.
    manager.functions().registerFunction< int >( InitialFunction( id, manager, addr ) );
    manager.functions().registerFunction< int, int >( Read( id, manager ) );
    manager.functions().registerFunction< int, int >( SendSave( id, manager ) );
    manager.functions().registerFunction< int, int >( Check( id, manager ) );

    // Register the memory implementation - the memory implementation is responsible for 
    // initiating memory-relevant communication, hence why the sender is passed too.
    manager.memory().useMemory( 
        std::make_unique< TestMemory >( id ) );

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
    distributionManagerBlockingMemory( );
    return 0;
}