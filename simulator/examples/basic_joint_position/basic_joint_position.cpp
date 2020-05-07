#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>

#include "rofi_hal.hpp"


int main()
{
    using namespace rofi::hal;
    using Callback = std::function< void() >;
    constexpr float pi = 3.141592741f;

    std::cout << "Starting basic position example\n";

    RoFI localRofi = RoFI::getLocalRoFI();
    Joint joint = localRofi.getJoint( 0 );

    auto minPos = std::clamp( joint.minPosition(), -pi, 0.f );
    auto maxPos = std::clamp( joint.maxPosition(), 0.f, pi );

    auto setPosAndWait = [ &joint,
                           speed = joint.maxSpeed() / 2,
                           delay = 500 ]( float position, Callback callback ) {
        std::cout << "Set position to " << position << std::endl;
        joint.setPosition( position, speed, [ & ]( Joint ) {
            RoFI::wait( delay, std::move( callback ) );
        } );
    };


    while ( true )
    {
        std::promise< void > endCyclePromise;
        auto endCycleFuture = endCyclePromise.get_future();
        setPosAndWait( 0, [ & ] {
            setPosAndWait( minPos / 2, [ & ] {
                setPosAndWait( minPos, [ & ] {
                    setPosAndWait( maxPos / 2, [ & ] {
                        setPosAndWait( maxPos, [ & ] {
                            setPosAndWait( minPos, [ & ] {
                                setPosAndWait( maxPos, [ & ] { endCyclePromise.set_value(); } );
                            } );
                        } );
                    } );
                } );
            } );
        } );

        endCycleFuture.get();
    }

    std::cout << "Basic velocity position ended\n";
}
