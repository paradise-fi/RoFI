#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributedTaskManager.hpp"
#include "initial.hpp"
#include "add.hpp"
#include "multiply.hpp"
#include "delay.hpp"
#include "implementation/replicatedMemory.hpp"

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

int currentCount = 1;
std::map< int, std::string > dataMap;

void startElectionProtocol() {
    std::cout << "Starting crash tolerant election test\n";
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

    DistributedTaskManager manager(
        std::move( election ), addr,
        reinterpret_cast< MessageDistributor* >( messageDistributor ), std::move( pcb ) );
    
    manager.memoryService().useMemory( std::make_unique< ReplicatedMemory >() );
    
    std::unique_ptr< DistributedFunction< int > > initial = std::make_unique< Initial >( id, manager );
    std::unique_ptr< DistributedFunction< int, int > > add = std::make_unique< Add >( currentCount, manager );
    std::unique_ptr< DistributedFunction< int, int > > delay = std::make_unique< Delay >( currentCount, manager );
    std::unique_ptr< DistributedFunction< int, int > > multiply = std::make_unique< Multiply >( currentCount, manager );
    std::unique_ptr< DistributedFunction< Ip6Addr > > barrier = std::make_unique< NaiveBarrier >( addr, manager );
    manager.functionRegistry().registerFunction< int >( std::move( initial ) );
    manager.functionRegistry().registerFunction< int, int >( std::move( add ) );
    manager.functionRegistry().registerFunction< int, int >( std::move( delay ) );
    manager.functionRegistry().registerFunction< int, int >( std::move( multiply ) );
    manager.functionRegistry().registerFunction< Ip6Addr >( std::move( barrier ) );

    manager.functionRegistry().setInitialTask( 0 );
    manager.start( id );

    while ( true ){
        sleep( 1 );
        manager.doWork();
        
        auto mem1 = manager.memoryService().readData( 1 );
        if ( mem1.success )
        {
            std::cout << "Data at address 1: " << mem1.data< int >() << std::endl;
        }

        auto mem2 = manager.memoryService().readData( 2 );
        if ( mem2.success )
        {
            std::cout << "Data at address 2: " << mem2.data< int >() << std::endl;
        }

        auto mem3 = manager.memoryService().readData( 3 );
        if ( mem3.success )
        {
            std::cout << "Data at address 3: " << mem3.data< int >() << std::endl;
        }
        // sleep( id );
    }
}

int main( void ) {
    startElectionProtocol( );
    return 0;
}