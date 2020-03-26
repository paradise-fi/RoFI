#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>

#include "rofi_hal.hpp"

using namespace rofi::hal;

using Callback = std::function< void() >;
using FunWithCallback = std::function< void( Callback ) >;

FunWithCallback addCallback( FunWithCallback f )
{
    return f;
}

FunWithCallback addCallback( std::function< void() > f )
{
    return [ f = std::move( f ) ]( Callback cb )
        {
            f();
            cb();
        };
}

template < typename F >
FunWithCallback operator>>( FunWithCallback lhs, F rhs )
{
    return [ lhs = std::move( lhs ), rhs = std::move( rhs ) ]( Callback cb )
        {
            lhs( std::bind( addCallback( rhs ), cb ) );
        };
}

template < typename F >
FunWithCallback operator>>( std::function< void() > lhs, F rhs )
{
    return addCallback( lhs ) >> rhs;
}

FunWithCallback setPosition( Joint joint, double pos, double speed = 1.5 )
{
    return [ = ]( Callback cb )
        {
            Joint( joint ).setPosition( pos, speed, [ cb = std::move( cb ) ]( Joint ){ cb(); } );
        };
}

std::function< void() > printStr( std::string str )
{
    static std::mutex printMutex;
    return [ str = std::move( str ) ]()
        {
            std::lock_guard< std::mutex > lock( printMutex );
            std::cout << str << std::flush;
        };
}

std::function< void() > loop( FunWithCallback f )
{
    return [ f = std::move( f ) ](){
            f( loop( f ) );
        };
}

int main()
{
    constexpr double deg90 = 1.5708;

    auto moveLoop = []( RoFI & rofi )
        {
            using std::to_string;
            const std::string rofiName = "RoFI " + to_string( rofi.getId() ) + ": ";
            auto moveSequence = printStr( rofiName + "1\n" )
                    >> setPosition( rofi.getJoint( 0 ), 3.14 )
                    >> printStr( rofiName + "2\n" )
                    >> setPosition( rofi.getJoint( 1 ), deg90 )
                    >> printStr( rofiName + "3\n" )
                    >> setPosition( rofi.getJoint( 2 ), deg90 )
                    >> printStr( rofiName + "4\n" )
                    >> setPosition( rofi.getJoint( 0 ), 0 )
                    >> printStr( rofiName + "5\n" )
                    >> setPosition( rofi.getJoint( 1 ), -deg90 )
                    >> printStr( rofiName + "6\n" )
                    >> setPosition( rofi.getJoint( 2 ), -deg90 );
            std::thread( loop( moveSequence ) ).detach();
        };


    RoFI & localRofi = RoFI::getLocalRoFI();
    moveLoop( localRofi );

    while ( true )
    {
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    }
}
