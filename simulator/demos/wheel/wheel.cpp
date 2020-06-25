#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

#include "rofi_hal.hpp"
#include "to_array.hpp"


using namespace rofi::hal;

constexpr auto rofiIds = to_array( { 1, 2, 3, 4, 5 } );
constexpr auto rofiIds2 = to_array( { 6, 7, 8, 9, 10 } );
constexpr auto angles =
        to_array( { 0., 1.0472, 1.0472, 1.0472, 0., 0., 1.0472, 1.0472, 1.0472, 0. } );

static_assert( angles.size() == 2 * rofiIds.size() );
static_assert( angles.size() == 2 * rofiIds.size() );

constexpr double velocity = 1;


template < typename C >
std::pair< std::map< RoFI::Id, RoFI >, std::vector< Joint > > getRofisAndJoints( const C & rofiIds )
{
    std::pair< std::map< RoFI::Id, RoFI >, std::vector< Joint > > result;
    auto & [ rofis, joints ] = result;

    for ( auto id : rofiIds )
    {
        rofis.emplace( id, RoFI::getRemoteRoFI( id ) );
    }
    assert( rofis.size() == rofiIds.size() );

    for ( auto rofiPair : rofis )
    {
        assert( rofiPair.second.getDescriptor().jointCount == 3 );
        rofiPair.second.getJoint( 2 ).setPosition( 0, 1, nullptr );

        joints.push_back( rofiPair.second.getJoint( 0 ) );
        joints.push_back( rofiPair.second.getJoint( 1 ) );
    }

    assert( joints.size() == angles.size() );

    return result;
}

int main()
{
    std::cout << "Starting wheel example\n";

    auto [ rofis, joints ] = getRofisAndJoints( rofiIds );
    auto [ rofis2, joints2 ] = getRofisAndJoints( rofiIds2 );

    std::atomic_bool finishedFlag = false;
    int i = 0;
    while ( true )
    {
        finishedFlag = false;
        i++;
        std::cout << "Wheel cycle: " << i << std::endl;

        for ( int j = 0; j < static_cast< int >( angles.size() ); j++ )
        {
            int rotated = ( j + i ) % angles.size();
            assert( rotated >= 0 && rotated < static_cast< int >( joints.size() ) );

            if ( j == 0 )
            {
                joints.at( rotated ).setPosition(
                        angles[ j ],
                        velocity,
                        [ &finishedFlag ]( Joint ) { finishedFlag = true; } );

                joints2.at( rotated ).setPosition( angles[ j ], velocity, nullptr );
                continue;
            }

            joints.at( rotated ).setPosition( angles[ j ], velocity, nullptr );
            joints2.at( rotated ).setPosition( angles[ j ], velocity, nullptr );
        }

        while ( !finishedFlag )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
        }
    }
}
