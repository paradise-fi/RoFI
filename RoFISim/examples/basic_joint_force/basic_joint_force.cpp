#include <future>
#include <iostream>

#include "rofi_hal.hpp"


int main()
{
    using namespace rofi::hal;

    std::cout << "Starting basic force example\n";

    RoFI & localRofi = RoFI::getLocalRoFI();
    auto torque = 0.5 * localRofi.getJoint( 2 ).maxTorque();

    while ( true )
    {
        std::promise< void > endCyclePromise;
        auto endCycleFuture = endCyclePromise.get_future();

        localRofi.getJoint( 2 ).setTorque( torque );
        RoFI::wait( 1000, [ & ] {
            localRofi.getJoint( 2 ).setTorque( -torque );
            RoFI::wait( 1000, [ & ] {
                localRofi.getJoint( 2 ).setTorque( 0 );
                RoFI::wait( 1000, [ & ] { endCyclePromise.set_value(); } );
            } );
        } );

        endCycleFuture.get();
    }
}
