#include <gazebo/gazebo_client.hh>

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

#include "rofi_hal.hpp"

using rofi::hal::RoFI;

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

FunWithCallback setPosition( RoFI::Joint joint, double pos, double speed = 1.5 )
{
    return [ = ]( Callback cb ){
        RoFI::Joint( joint ).setPosition( pos, speed, [ = ]( RoFI::Joint ){ cb(); } );
    };
}

std::function< void( void ) > printStr( const std::string & str )
{
    return [ = ](){
        std::cout << str;
        std::cout.flush();
    };
}

std::function< void( void ) > loop( FunWithCallback f )
{
    return [ f ](){ f( loop( f ) ); };
}

int main( int argc, char **argv )
{
    using rofi::hal::RoFI;

    gazebo::client::setup( argc, argv );

    RoFI & rofi = RoFI::getLocalRoFI();

    constexpr double deg90 = 1.5708;

    auto sequence = printStr( "1\n" )
            >> setPosition( rofi.getJoint( 0 ), 3.14 )
            >> printStr( "2\n" )
            >> setPosition( rofi.getJoint( 1 ), deg90 )
            >> printStr( "3\n" )
            >> setPosition( rofi.getJoint( 2 ), deg90 )
            >> printStr( "4\n" )
            >> setPosition( rofi.getJoint( 0 ), 0 )
            >> printStr( "5\n" )
            >> setPosition( rofi.getJoint( 1 ), -deg90 )
            >> printStr( "6\n" )
            >> setPosition( rofi.getJoint( 2 ), -deg90 );

    auto loop1 = loop( sequence );

    loop1();

    while ( true )
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
}
