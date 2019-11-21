#include <gazebo/gazebo_client.hh>

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

#include "rofi_hal.hpp"

using rofi::hal::RoFI;
using rofi::hal::Joint;

using Callback = std::function< void( void ) >;
using FunWithCallback = std::function< void( Callback ) >;

FunWithCallback addCallback( FunWithCallback f )
{
    return f;
}

FunWithCallback addCallback( std::function< void( void ) > f )
{
    return [ f ]( Callback cb ){
        f();
        cb();
    };
}

template < typename F >
FunWithCallback operator>>( FunWithCallback lhs, F rhs )
{
    return [=]( Callback cb ){ lhs( std::bind( addCallback( rhs ), cb ) ); };
}

template < typename F >
FunWithCallback operator>>( std::function< void( void ) > lhs, F rhs )
{
    return addCallback( lhs ) >> rhs;
}

std::function< void( void ) > printStr( std::string str )
{
    static std::mutex printMutex;
    return [ str = std::move( str ) ](){
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
    return [ prefix = std::move( prefix ) ]( rofi::hal::Connector, rofi::hal::Connector::Packet packet )
        {
            printStr( std::move( prefix ) + toContainer< std::string >( packet ) + "\n" )();
        };
}

int main( int argc, char **argv )
{
    using rofi::hal::RoFI;

    gazebo::client::setup( argc, argv );

    RoFI & localRofi = RoFI::getLocalRoFI();
    try
    {
        using std::to_string;
        int i = 0;
        while ( true )
        {
            localRofi.getConnector( i ).connect();
            auto callback = onPacketCallback( "localRoFI connector " + to_string( i ) + ":\n" );
            localRofi.getConnector( i ).onPacket( std::move( callback ) );
            auto pingLoop = []( rofi::hal::Connector connector ){
                while ( true )
                {
                    sendPacket( connector, "ping" );
                    std::this_thread::sleep_for( std::chrono::seconds( 6 ) );
                }
            };
            std::thread( std::move( pingLoop ), localRofi.getConnector( i ) ).detach();
            i++;
            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        }
    }
    catch ( const std::exception & e ) {}

    RoFI & otherRofi = RoFI::getRemoteRoFI( 1 );
    try
    {
        using std::to_string;
        int i = 0;
        while ( true )
        {
            otherRofi.getConnector( i ).connect();
            auto callback = onPacketCallback( "otherRoFI connector " + to_string( i ) + ":\n" );
            otherRofi.getConnector( i ).onPacket( std::move( callback ) );
            auto pingLoop = []( rofi::hal::Connector connector ){
                while ( true )
                {
                    sendPacket( connector, "ping" );
                    std::this_thread::sleep_for( std::chrono::seconds( 6 ) );
                }
            };
            std::thread( std::move( pingLoop ), otherRofi.getConnector( i ) ).detach();
            i++;
            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        }
    }
    catch ( const std::exception & e ) {}


    while ( true )
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
}
