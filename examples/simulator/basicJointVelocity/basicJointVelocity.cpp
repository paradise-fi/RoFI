#include <future>
#include <iostream>

#include "rofi_hal.hpp"


int main()
{
    using namespace rofi::hal;

    std::cout << "Starting basic velocity example\n";

    RoFI localRofi = RoFI::getLocalRoFI();
    auto speed = 0.5f * localRofi.getJoint( 2 ).maxSpeed();

    while ( true )
    {
        std::promise< void > endCyclePromise;
        auto endCycleFuture = endCyclePromise.get_future();

        localRofi.getJoint( 2 ).setVelocity( speed );
        RoFI::postpone( 1000, [ & ] {
            localRofi.getJoint( 2 ).setVelocity( -speed );
            RoFI::postpone( 1000, [ & ] {
                localRofi.getJoint( 2 ).setVelocity( 0 );
                RoFI::postpone( 1000, [ & ] { endCyclePromise.set_value(); } );
            } );
        } );

        endCycleFuture.get();
    }
}
