#include "rofi_hal.hpp"

#include <iostream>
#include <future>


int main()
{
    using namespace rofi::hal;

    std::cout << "Starting basic force example\n";

    std::promise< void > endPromise;
    auto endFuture = endPromise.get_future();

    RoFI & localRofi = RoFI::getLocalRoFI();
    auto torque = 0.5 * localRofi.getJoint( 0 ).maxTorque();

    while ( true )
    {
        std::promise< void > endCyclePromise;
        auto endCycleFuture = endCyclePromise.get_future();

        localRofi.getJoint( 0 ).setTorque( torque );
        RoFI::wait( 1000, [&]{
            localRofi.getJoint( 0 ).setTorque( -torque );
            RoFI::wait( 1000, [&]{
                localRofi.getJoint( 0 ).setTorque( 0 );
                RoFI::wait( 1000, [&]{
                    endCyclePromise.set_value();
                } );
            } );
        } );

        endCycleFuture.get();
    }

    std::cout << "Basic force example ended\n";
}
