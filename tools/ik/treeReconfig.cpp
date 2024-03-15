#include <chrono>
#include <stdexcept>
#include <treeReconfig.hpp>

#include <future>
#include <thread>

#include <atoms/cmdline_utils.hpp>
#include <atoms/result.hpp>
#include <atoms/units.hpp>
#include <legacy/configuration/Configuration.h>
#include <configuration/serialization.hpp>
#include <parsing/parsing_lite.hpp>

#include <rofi_hal.hpp>

#include "dimcli/cli.h"

using namespace rofi::configuration;
using namespace rofi::reconfiguration;
using namespace std::chrono_literals;

namespace hal = rofi::hal;

int main( int argc, char** argv ){

    Dim::Cli cli;
    auto& sim = cli.opt< bool >( "s simulate" ).desc( "Simulate movement" );
    auto& old = cli.opt< bool >( "l legacy" ).desc( "Legacy configurations rooted at module 0" );
    auto& in = cli.opt< std::string >( "i input", "" ).desc( "Input path" );
    auto& out = cli.opt< std::string >( "o output", "" ).desc( "Output path" );
    auto& snake = cli.opt< bool > ( "snake" ).desc( "Reconfigure via snake" );
    auto& length = cli.opt< int >( "n max_length ", 4 ).desc( "Max shoulder length" );

    if( !cli.parse( argc, argv ) )
        return cli.printError( std::cerr );

    std::ifstream in_file( *in );
    RofiWorld initial;

    std::ifstream out_file( *out );
    RofiWorld target;
    if( *old ){
        initial = readOldConfigurationFormat( in_file );
        connect< RigidJoint >( initial.getModule( 0 )->components()[ 6 ], Vector{ 0, 0, 0 }, identity );
        target = readOldConfigurationFormat( out_file );
        connect< RigidJoint >( target.getModule( 0 )->components()[ 6 ], Vector{ 0, 0, 0 }, identity );
    } else {
        initial = *atoms::readInput( *in, rofi::parsing::parseJson )
                .and_then( rofi::parsing::getRofiWorldFromJson );
        target = *atoms::readInput( *out, rofi::parsing::parseJson )
                .and_then( rofi::parsing::getRofiWorldFromJson );
    }
    // auto world = from_tree( tree.tree );
    // auto world = from_tree( s.intermediate_tree( s.tree, c.tree ) );
    if( *sim ){
        for ( [[maybe_unused]] const auto& module : initial.modules() ){
            if( std::system( "getlocal" ) != 0 ){
                return 1;
            }
        }
    }
    TreeConfiguration start( initial, 0, *sim, {}, *length );
    TreeConfiguration target_tree( target );

    std::cout << "target:" << serialization::toJSON( target_tree.world ) << '\n';
    initial.prepare().get_or_throw_as<std::logic_error>();
    target.prepare().get_or_throw_as<std::logic_error>();

    auto begin = std::chrono::high_resolution_clock::now();
    // reconfigure( spider, chair );
    if( !(*snake ) ){
        auto [ s_center, t_center ] = mcs_naive( initial, target );
        start = TreeConfiguration( initial, s_center, *sim, {}, *length );
        std::cout << "starting: " << serialization::toJSON( start.world ) << '\n';
        start.reconfigure( target );
        std::cout << "Connections: " << start.connections << '\n';
        std::cout << "Movement: " << start.movement << '\n';
    } else {
        start.max_length = 10;
        start.reconfigure_snake();
        target_tree.max_length = 10;
        target_tree.reconfigure_snake();
        std::cout << "Connections: " << start.connections + target_tree.connections << '\n';
        std::cout << "Movement: " << start.movement + target_tree.movement << '\n';
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::seconds >( end - begin );
    std::cout << "Time: " << duration.count() << '\n';
    // s.reconfigure_snake();
}