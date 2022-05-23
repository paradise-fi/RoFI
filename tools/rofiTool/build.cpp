#include "build.hpp"
#include "rendering.hpp"
#include <configuration/pad.hpp>
#include <configuration/bots/umpad.hpp>
#include <configuration/universalModule.hpp>

static auto command = Dim::Cli().command( "build" )
    .desc( "Build and show given rofibot" );
static auto& botType = command.opt< int >( "<ROFIBOT>" )
    .desc("Specify wished rofibot: 0 - UM, 1 - UM n pad, 2 - UM n x m pad,"
            + std::string( " 3 - RoFiCoM n pad, 4 - RoFiCoM n x m pad" ) );

enum class BotType {
                     UM
                   , UMpadN
                   , UMpadNM
                   , NPad
                   , NMPad
};

int readIntWithMsg( const std::string& msg ) {
    int n;
    std::cout << msg;
    std::cin >> n;
    return n;
}

rofi::configuration::RofiWorld buildWishedRofibot( BotType botType ) {
    using namespace rofi::configuration;
    RofiWorld world;
    int n, m;
    switch ( botType ) {
        case BotType::UM:
            world.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
            return world;
        case BotType::UMpadN:
            n = readIntWithMsg( "Dimension: " );
            return buildUMpad( n );
        case BotType::UMpadNM:
            n = readIntWithMsg( "Dimension n: " );
            m = readIntWithMsg( "Dimension m: " );
            return buildUMpad( n, m );
        case BotType::NPad:
            n = readIntWithMsg( "Dimension: " );
            world.insert( Pad( 0, n ) );
            break;
        case BotType::NMPad:
            n = readIntWithMsg( "Dimension n: " );
            m = readIntWithMsg( "Dimension m: " );
            world.insert( Pad( 0, n, m ) );
            break;
        default:
            throw std::runtime_error( "Unknown model" );
    }
    return world;
}

std::string botTypeToString( BotType b ) {
    std::string str;
    switch ( b ) {
        case BotType::UM:
            str = "an universal module";
            break;
        case BotType::UMpadN:
        case BotType::UMpadNM:
            str = "a pad of universale modules";
            break;
        default:
            str = "a RoFICoM pad";
    }
    return str;
}

int build( Dim::Cli & /* cli */ ) {
    auto world = buildWishedRofibot( static_cast< BotType >( *botType ) );

    rofi::configuration::connect< rofi::configuration::RigidJoint >(
        *botType > 2 ? world.getModule( 0 )->connectors()[ 0 ] // No body within the pad
                     : world.getModule( 0 )->bodies()[ 0 ],     // UM, so we fix its body
        rofi::configuration::matrices::Vector( { 0, 0, 0 } ),
        rofi::configuration::matrices::identity );

    world.prepare();
    renderConfiguration( world, botTypeToString( static_cast< BotType >( *botType ) ) );

    return 0;
}
