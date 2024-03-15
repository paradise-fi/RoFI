#include <algorithm>
#include <cfloat>
#include <future>
#include <iostream>

#include <atoms/cmdline_utils.hpp>
#include <atoms/units.hpp>
#include <legacy/configuration/Configuration.h>
#include <configuration/serialization.hpp>
#include <parsing/parsing_lite.hpp>

#include <rofi_hal.hpp>
#include <planning.hpp>
#include <thread>

#include "dimcli/cli.h"

namespace hal = rofi::hal;
using namespace rofi::geometry;
using namespace rofi::kinematics;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace std::chrono_literals;

/*
  Set a shoulder consisting of RoFI handles (modules within the simulation)
  to the target position. In order to make the animation smoother and have
  the end effector move at a relatively constant speed, each joint moves at
  a speed that correlates to how much it changes the position of the end
  effector.
 */


void setConfiguration( //std::vector< hal::RoFI >& shoulder, 
                       Manipulator& manipulator,
                       const RofiWorld& next, const std::vector< hal::RoFI >& rofis )
{
    auto diffs = manipulator.relative_changes( next );
    double max_diff = *std::max_element( diffs.begin(), diffs.end() );
    std::vector< bool > promises;//( shoulder.size() * 3, false );
    
    size_t promise_idx = 0;
    for( size_t i = 0; i < manipulator.arm.size(); i++ ){
        const auto& body = manipulator.arm[ i ];
        auto rofi = rofis[ body.id ];
        auto j = body.side == A ? Alpha : Beta;
        
        float speed = diffs[ i ] / max_diff;

        promises.emplace_back( false );
        rofi.getJoint( j ).setPosition( get_rotation( next, body.id, j ).rad(), std::max( 0.2f, speed ),
                                        [&, promise_idx]( hal::Joint ){ promises[ promise_idx ] = true; } );
        ++promise_idx;

        if( i != 0 && manipulator.arm[ i ].id == manipulator.arm[ i - 1 ].id ){
            promises.emplace_back( false );
            rofi.getJoint( Gamma ).setPosition( get_rotation( next, body.id, Gamma ).rad(), std::max( 0.2f, speed ), 
                                            [&, promise_idx]( hal::Joint ){ promises[ promise_idx ] = true; } );
            ++promise_idx;
        }
    }
    while( std::any_of( promises.begin(), promises.end(), []( bool finished ){ return !finished; } ) ){
        std::this_thread::sleep_for(10ms);
    }

}

/* Try to reach a target, and simulate the resulting motion */
bool reach_target( std::vector< hal::RoFI >& shoulder, Manipulator& manipulator,
                   const Matrix& target, bool sim, RofiWorld last )
{
    auto start = std::chrono::high_resolution_clock::now();
    auto positions = manipulator.reach( target );
    if( positions.empty() ){
        std::cerr << "Reaching target failed\n";
        return false;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast< std::chrono::microseconds >( end - start );
    std::cout << "Target reached in " << diff.count() << " mis\n";
    if( sim ){
        for( auto p : positions ){
            //std::cout << serialization::toJSON( p );
            //getchar();
            setConfiguration( manipulator, p, shoulder );
            last = p;
        }
    }
    //std::cout << serialization::toJSON( positions.back() );
    return true;
}

int main( int argc, char** argv ){
    Dim::Cli cli;
    auto& in = cli.opt< std::string >( "<PATH>" ).desc( "Path to input configuration" );
    auto& sim = cli.opt< bool >( "s simulate" ).desc( "Simulate movement" );
    auto& example = cli.opt< std::string >( "e example" ).desc( "Run one of the examples:"
                                                         "Wall, Hole, Cluster, Around" );

    if( !cli.parse( argc, argv ) )
        return cli.printError( std::cerr );


    auto json = atoms::readInput( *in, rofi::parsing::parseJson );

    auto world = serialization::fromJSON( *json );
    std::cout << "prepared: " << std::boolalpha << world.prepare().has_value() << '\n';

    Manipulator manipulator( world );
    manipulator.world.prepare();
    auto last = world;

    //copy.world.prepare();
    // for( size_t i = 0; i < manipulator.get_arm().size() / 2; i++ ){
    //     if( std::system( "getlocal" ) != 0 ){
    //         return 1;
    //     };
    // }
    // //std::map< int, hal::RoFI > rofis;
    // std::vector< hal::RoFI > rofis;
    // for( int i = 0; i < 4; i++ ){
    //     rofis.push_back( hal::RoFI::getRemoteRoFI( i ) );
    // }
    //setConfiguration( manipulator, copy.world, rofis );

    
    Matrix src = manipulator.joint_position( manipulator.arm.size() - 1 );
    
    // Example targets
    Matrix around1 = src * translate( { 3, 0, 3 } )
        * rotate( M_PI, { 1, 0, 0 } )
        * rotate( M_PI_2, { 0, 0, 1 } );
    Matrix around2 = src * translate( { 2, -2, 3 } )
        * rotate( M_PI, { 1, 0, 0 } )
        * rotate( M_PI_2, { 0, 0, 1 } );
    Matrix hole = src * translate( { 1, 3.5, 4 } )
        * rotate( M_PI_2, { 1, 0, 0 } );
        // * rotate( M_PI_2, { 0, 0, 1 } );

    Matrix cluster = src * translate( { -2.5, 3.0, 5.5 } )
        * rotate( -M_PI_2, { 1, 0, 0 } )
        * rotate( -M_PI / 3, { 0, 0, 1 } );

    Matrix cluster2 = src * translate( { -3, 1.5, 3 } )
        * rotate( M_PI, { 1, 0, 0 } )
        * rotate( M_PI_2, { 0, 0, 1 } );

    std::vector< hal::RoFI > shoulder;
    if( *sim ){
        // Hack to deal with getRemoteRoFI not responding when the module hasn't been accessed
        for( size_t i = 0; i < manipulator.get_arm().size() / 2; i++ ){
            if( std::system( "getlocal" ) != 0 ){
                return 1;
            }
        }
        for( size_t i = 0; i < manipulator.get_arm().size() / 2; i++ ){
            shoulder.push_back( hal::RoFI::getRemoteRoFI( int( i ) ) );
        }
    }

    if( *example == "Wall" ){
        reach_target( shoulder, manipulator, around1, *sim, last );
        //manipulator = Manipulator( manipulator.world );
        reach_target( shoulder, manipulator, around2, *sim, last );
    } else if( *example == "Hole" ){
        reach_target( shoulder, manipulator, hole, *sim, last );
    } else if( *example == "Cluster" ){
        reach_target( shoulder, manipulator, cluster, *sim, last );
    } else if( *example == "Around" ) {
        reach_target( shoulder, manipulator, cluster2, *sim, last );
    } else {
        while( true ){
            std::cout << "Provide target with respect to the current end-effector, or cancel with Ctrl-C:\n"
                "Format: x y z xr yr zr, representing position and rotation around each axis (in radians)\n";
            std::vector< double > values( 6, 0.0 );
            for( int i = 0; i < 6; i++ ){
                std::cin >> values[ i ];
            }
            for( int i = 0; i < 6; i++ ){
                std::cout << values[ i ];
            }
            
            Matrix src = manipulator.joint_position( manipulator.arm.size() - 1 );

            Matrix target = src * translate( { values[ 0 ], values[ 1 ], values[ 2 ] } )
                * rotate( values[ 3 ], { 1, 0, 0 } )
                * rotate( values[ 4 ], { 0, 1, 0 } )
                * rotate( values[ 5 ], { 0, 0, 1 } );

            reach_target( shoulder, manipulator, target, *sim, last );
        }
    }
    
}
