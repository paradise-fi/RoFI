#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>

#include <lwip/udp.h>
#include <iostream>
#include <string>

#include "distributionManager.hpp"

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

int currentCount = 0;
std::map< int, std::string > dataMap;

int count()
{
    return currentCount++;
}

int multiplyBy2( int number )
{
    return number * 2; 
}

int initialFunction( void )
{
    std::cout << "Initial Function!" << std::endl;
    return RoFI::getLocalRoFI().getId();
}

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
    
    manager.useMemory( std::make_unique< ReplicatedMemoryManager >( net, reinterpret_cast< MessageDistributor* >( messageDistributor ), addr, manager.getSender() ) );
    
    std::function< void ( std::optional< int >, const rofi::hal::Ip6Addr& ) > initReaction = 
        [&]( std::optional< int > moduleId, const Ip6Addr& sender ) 
        { 
            std::cout << "[InitReaction] Received initial response from " << moduleId.value() << std::endl;
            if ( !moduleId.has_value() )
            {
                std::cout << "[InitReaction] Something went wrong." << std::endl;
            }
            if ( moduleId.value() % 2 == 0 )
            {
                int modId = moduleId.value();
                manager.pushTask< int, int >(sender, 1, 1, std::make_tuple< int >( std::move( modId ) ) );
                std::cout << "[InitReaction] Going to save " << id << std::endl;
                manager.saveData(reinterpret_cast< uint8_t* >( &id ), sizeof( int ), id );
            }
            else
            {
                manager.pushTask< int >(sender, 2, 1, std::tuple< int >() );
            }
        };
    
    manager.registerFunction< int >( 0, std::function< int () >(initialFunction), initReaction );
    manager.registerFunction< int, int >( 1,
        std::function< int ( int ) >( [&]( int number ) 
        { 
            int value = number * 2;

            int save = id * 100 + number;
            std::cout << "[MAIN] Saving Data in main.cpp multiply: " << save << std::endl;
            manager.saveData( reinterpret_cast< uint8_t* >( &save ), sizeof( int ), id );
            return value;
        } ), 
        [&]( std::optional< int > result, const Ip6Addr& sender )
        {
            std::cout << "[MAIN] Multiply result from " << sender << " is " << ( result.has_value() ? result.value() : -1 ) << std::endl;
            manager.pushTask< int >( sender, 2, 1, std::tuple< int >() );
        }
    );

    manager.registerFunction< int >( 2, 
        [&]() 
        { 
            int save = id * 100 + currentCount;
            std::cout << "[MAIN] Saving Data in main.cpp count: " << save << std::endl;
            manager.saveData(reinterpret_cast< uint8_t* >( &save ), sizeof( int ), id);
            return currentCount++;
        }, 
        [&]( std::optional< int > result, const Ip6Addr& sender )
        {
            std::cout << "[MAIN] Increment result from " << sender << "is " << ( result.has_value() ? result.value() : -1 ) << std::endl;
            if ( result.has_value() && result.value() < 5 )
            {
                manager.pushTask< int >( sender, 2, 1, std::tuple< int >() );
            } 
        }
    );

    manager.setInitialTask( 0 );
    manager.start( id );

    while (true){
        sleep( 1 );
        manager.doWork();
        
        int mem1 = 0;
        if (manager.readData(1, mem1))
        {
            std::cout << "Data at address 1: " << mem1 << std::endl;
        }

        int mem2 = 0;
        if (manager.readData(2, mem2))
        {
            std::cout << "Data at address 2: " << mem2 << std::endl;
        }

        int mem3 = 0;
        if (manager.readData(3, mem3))
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