#include <algorithm>
#include <cassert>
#include <cmath>
#include <future>
#include <iostream>

#include "rofi_hal.hpp"

using namespace rofi::hal;

// auxiliary function to help referencing of modules
int id(int v) { return v-1; }

int main()
{
    // Hexapod contains 16 modules, with ids 1 - 17, see hexapod.in.
    // Shape of the hexapod is as follows.
    //
    //             (head)
    //
    //             16A 17B
    // 06A=06B-05A=05B-11A=11B-12A=12B 
    //             15A=15B
    // 04A=04B-03A=03B-09A=09B-10A=10B 
    //             14A=14B
    // 02A=02B-01A=01B-07A=07B-08A=08B 
    //             13A=13B
    //
    //             (tail)
    //
    // Positive values of joints in 01A,03A,05A,07B,09B,11B go towards the head.
    // Positive values of joints in 02B,04B,06B,08A,10A,12A go towards the ground.
    // Shoes A contain alpha joints, shoes B contain beta joints.

    constexpr float pi = 3.141592741f;
    
    const int gamma = 0;
    const int alpha = 1;
    const int beta  = 2;


    std::vector< RoFI > modules;
    for (int i = 1; i <= 17; i++) {
      modules.push_back( RoFI::getRemoteRoFI(i) );
    }

    const int speed = modules[id(1)].getJoint(1).maxSpeed() / 2;
    const auto step = 0.3;
  
    // We will decompose the movement into 6 phases.
    int init_phase = 1;
    int last_phase = 6;
    int phase = init_phase;
    
    
    // Raise legs 1-2 9-10 and 5-6
    //----------------------------
    modules[id(02)].getJoint(beta).setPosition( step, speed, NULL );
    modules[id(06)].getJoint(beta).setPosition( step, speed, NULL );
    modules[id(10)].getJoint(alpha).setPosition( step, speed, NULL );
    
    while (true)
    {
        // -------------------------------------
        // decide what to do
        // -------------------------------------

        // -------------------------------------
        // do it
        // -------------------------------------

        // -------------------------------------
        // wait for finishing the action
        // -------------------------------------

        phase ++;
        if (phase > last_phase) { phase = init_phase; }
    }
        
    
    // RoFI localRofi = RoFI::getLocalRoFI();
    // int jointCount = localRofi.getDescriptor().jointCount;
    // assert( jointCount > 0 );
    // Joint joint = localRofi.getJoint( 0 );

    // const auto minPos = std::clamp( joint.minPosition(), -pi, 0.f );
    // const auto maxPos = std::clamp( joint.maxPosition(), 0.f, pi );

    // const int delayMs = 1000;
    // const int speed = joint.maxSpeed() / 2;

    // while ( true )
    // {
    //     std::promise< void > endCyclePromise;
    //     auto endCycleFuture = endCyclePromise.get_future();

    //     joint.setPosition( 0, speed, [ & ]( Joint ) {
    //         RoFI::wait( delayMs, [ & ] {
    //             joint.setPosition( minPos / 2, speed, [ & ]( Joint ) {
    //                 RoFI::wait( delayMs, [ & ] {
    //                     joint.setPosition( minPos * 9.f / 10, speed, [ & ]( Joint ) {
    //                         RoFI::wait( delayMs, [ & ] {
    //                             joint.setPosition( maxPos / 2, speed, [ & ]( Joint ) {
    //                                 RoFI::wait( delayMs, [ & ] {
    //                                     joint.setPosition( maxPos * 9.f / 10,
    //                                                        speed,
    //                                                        [ & ]( Joint ) {
    //                                                            RoFI::wait( delayMs, [ & ] {
    //                                                                endCyclePromise.set_value();
    //                                                            } );
    //                                                        } );
    //                                 } );
    //                             } );
    //                         } );
    //                     } );
    //                 } );
    //             } );
    //         } );
    //     } );

    //     endCycleFuture.get();
    // }

    return 0;
}
