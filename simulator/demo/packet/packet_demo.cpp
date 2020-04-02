#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "rofi_hal.hpp"

using rofi::hal::Joint;
using rofi::hal::RoFI;

using Callback = std::function< void( void ) >;
using FunWithCallback = std::function< void( Callback ) >;

FunWithCallback addCallback( FunWithCallback f )
{
    return f;
}

FunWithCallback addCallback( std::function< void( void ) > f )
{
    return [ f ]( Callback cb ) {
        f();
        cb();
    };
}

template < typename F >
FunWithCallback operator>>( FunWithCallback lhs, F rhs )
{
    return [ = ]( Callback cb ) { lhs( std::bind( addCallback( rhs ), cb ) ); };
}

template < typename F >
FunWithCallback operator>>( std::function< void( void ) > lhs, F rhs )
{
    return addCallback( lhs ) >> rhs;
}

std::function< void( void ) > printStr( std::string str )
{
    static std::mutex printMutex;
    return [ str = std::move( str ) ]() {
        std::lock_guard< std::mutex > lock( printMutex );
        std::cout << str;
        std::cout.flush();
    };
}

template < typename To, typename From >
To toContainer( const From & from )
{
    To res;
    res.reserve( from.size() );
    for ( auto & elem : from )
    {
        res.push_back( static_cast< typename To::value_type >( elem ) );
    }

    return res;
}

void sendPacket( rofi::hal::Connector connector, const std::string & msg )
{
    connector.send( toContainer< rofi::hal::Connector::Packet >( msg ) );
}

auto onPacketCallback( std::string prefix )
{
    return [ prefix = std::move( prefix ) ]( rofi::hal::Connector,
                                             rofi::hal::Connector::Packet packet ) {
        printStr( std::move( prefix ) + toContainer< std::string >( packet ) + "\n" )();
    };
}

int main()
{
    using rofi::hal::RoFI;
    using namespace std::string_literals;

    RoFI & localRofi = RoFI::getLocalRoFI();
    RoFI & otherRofi = RoFI::getRemoteRoFI( 1 );
    auto connectors = { std::make_pair( localRofi.getConnector( 0 ), "Local RoFI (c. 0)"s ),
                        std::make_pair( otherRofi.getConnector( 3 ), "Remote RoFI (c. 3)"s ) };
    for ( auto [ connector, name ] : connectors )
    {
        connector.connect();
        printStr( "Extending " + name );
        connector.onPacket( onPacketCallback( name + " got message:\n" ) );
        auto pingLoop = [ name = std::move( name ) ]( rofi::hal::Connector connector ) {
            while ( true )
            {
                std::this_thread::sleep_for( std::chrono::seconds( 4 ) );
                sendPacket( connector, "ping from " + name );
            }
        };
        std::thread( std::move( pingLoop ), connector ).detach();
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    }

    while ( true )
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
}
