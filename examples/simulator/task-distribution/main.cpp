#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/rrp.hpp>
#include <networking/protocols/simpleReactive.hpp>

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

    int id = RoFI::getLocalRoFI().getId();
    std::cout << "This module is: " << id << "\n";

    NetworkManager net( RoFI::getLocalRoFI() );
    Ip6Addr addr = createAddress( id );
    
    net.addAddress( addr, 80, net.interface( "rl0" ) );
    net.setUp();

    auto rtProto = net.addProtocol( rofi::net::SimpleReactive() );
    net.setProtocol( *rtProto );

    DistributionManager manager( net, addr );
    std::function< void ( std::optional< int >, const rofi::hal::Ip6Addr& ) > initReaction = 
        [&]( std::optional< int > moduleId, const Ip6Addr& sender ) 
        { 
            std::cout << "Received initial response from " << moduleId.value() << std::endl;
            if ( !moduleId.has_value() )
            {
                std::cout << "Something went wrong." << std::endl;
            }
            if ( moduleId.value() % 2 == 0 )
            {
                int modId = moduleId.value();
                manager.pushTask< int, int >(sender, 1, std::make_tuple< int >( std::move( modId ) ) );
            }
            else
            {
                manager.pushTask< int >(sender, 2, std::tuple< int >() );
            }
        };
    
    manager.registerFunction< int >( 0, std::function< int () >(initialFunction), initReaction );
    manager.registerFunction< int, int >( 1, std::function< int ( int ) >( multiplyBy2 ), 
        [&]( std::optional< int > result, const Ip6Addr& sender )
        {
            std::cout << "Multiply result from " << sender << "is " << ( result.has_value() ? result.value() : -1 ) << std::endl;
            manager.pushTask< int >( sender, 2, std::tuple< int >() );
        }
    );

    manager.registerFunction< int >( 2, std::function< int ( ) >( count ), 
        [&]( std::optional< int > result, const Ip6Addr& sender )
        {
            std::cout << "Increment result from " << sender << "is " << ( result.has_value() ? result.value() : -1 ) << std::endl;
            if ( result.has_value() && result.value() < 10 )
            {
                manager.pushTask< int >( sender, 2, std::tuple< int >() );
            }
        }
    );

    manager.setInitialTask( 0 );
    manager.start( id );

    while (true){
        manager.doWork();
        sleep( id );
    }
}

int main( void ) {
    startElectionProtocol( );
    return 0;
}