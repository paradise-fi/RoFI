#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributionManager.hpp"
#include "initial.hpp"
#include "add.hpp"
#include "multiply.hpp"
#include "delay.hpp"

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
    
    DistributionManager manager( net, addr, reinterpret_cast< MessageDistributor* >( messageDistributor ), std::move( pcb ) );
    
    manager.useMemory( std::make_unique< ReplicatedMemoryManager >( reinterpret_cast< MessageDistributor* >( messageDistributor ), addr, manager.getSender() ) );
    
    std::unique_ptr< DistributedFunction< int > > initial = std::make_unique< Initial >( id, manager );
    std::unique_ptr< DistributedFunction< int, int > > add = std::make_unique< Add >( currentCount, manager );
    std::unique_ptr< DistributedFunction< int, int > > delay = std::make_unique< Delay >( currentCount, manager );
    std::unique_ptr< DistributedFunction< int, int > > multiply = std::make_unique< Multiply >( currentCount, manager );
    std::unique_ptr< DistributedFunction< Ip6Addr > > barrier = std::make_unique< NaiveBarrier >( addr, manager );
    manager.registerFunction< int >( std::move( initial ) );
    manager.registerFunction< int, int >( std::move( add ) );
    manager.registerFunction< int, int >( std::move( delay ) );
    manager.registerFunction< int, int >( std::move( multiply ) );
    manager.registerFunction< Ip6Addr >( std::move( barrier ) );

    manager.setInitialTask( 0 );
    manager.start( id );

    while ( true ){
        sleep( 1 );
        manager.doWork();
        
        int mem1 = 0;
        if ( manager.readData( 1, mem1 ) )
        {
            std::cout << "Data at address 1: " << mem1 << std::endl;
        }

        int mem2 = 0;
        if ( manager.readData( 2, mem2 ) )
        {
            std::cout << "Data at address 2: " << mem2 << std::endl;
        }

        int mem3 = 0;
        if ( manager.readData( 3, mem3 ) )
        {
            std::cout << "Data at address 3: " << mem3 << std::endl;
        }
        // sleep( id );
    }
}

int main( void ) {
    startElectionProtocol( );
    return 0;
}