#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <iostream>

#include "rofi_hal.hpp"


int main()
{
    using namespace rofi::hal;
    constexpr float pi = 3.141592741f;

    std::cout << "Starting basic position example\n";

    RoFI localRofi = RoFI::getLocalRoFI();
    int jointCount = localRofi.getDescriptor().jointCount;
    assert( jointCount > 0 );
    std::cout << "Using joint number " << ( jointCount >= 2 ? 1 : 0 ) << "\n";
    Joint joint = localRofi.getJoint( jointCount >= 2 ? 1 : 0 );

    const auto minPos = std::clamp( joint.minPosition(), -pi, 0.f );
    const auto maxPos = std::clamp( joint.maxPosition(), 0.f, pi );

    const int delayMs = 500;
    const int speed = joint.maxSpeed() / 2;

    while ( true )
    {
        std::promise< void > endCyclePromise;
        auto endCycleFuture = endCyclePromise.get_future();

        joint.setPosition( 0, speed, [ & ]( Joint ) {
            RoFI::wait( delayMs, [ & ] {
                joint.setPosition( minPos / 2, speed, [ & ]( Joint ) {
                    RoFI::wait( delayMs, [ & ] {
                        joint.setPosition( minPos, speed, [ & ]( Joint ) {
                            RoFI::wait( delayMs, [ & ] {
                                joint.setPosition( maxPos / 2, speed, [ & ]( Joint ) {
                                    RoFI::wait( delayMs, [ & ] {
                                        joint.setPosition( maxPos, speed, [ & ]( Joint ) {
                                            RoFI::wait( delayMs, [ & ] {
                                                joint.setPosition( minPos, speed, [ & ]( Joint ) {
                                                    RoFI::wait( delayMs, [ & ] {
                                                        joint.setPosition(
                                                                maxPos,
                                                                speed,
                                                                [ & ]( Joint ) {
                                                                    RoFI::wait( delayMs, [ & ] {
                                                                        endCyclePromise.set_value();
                                                                    } );
                                                                } );
                                                    } );
                                                } );
                                            } );
                                        } );
                                    } );
                                } );
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
