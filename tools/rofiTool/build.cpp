#include "build.hpp"
#include "rendering.hpp"
#include <pad.hpp>
#include <bots/umpad.hpp>

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

rofi::Rofibot buildWishedRofibot( BotType botType ) {
    rofi::Rofibot rofibot;
    int n, m;
    switch ( botType ) {
        case BotType::UM:
            rofibot.insert( rofi::buildUniversalModule( 0_deg, 0_deg, 0_deg ) );
            return rofibot;
        case BotType::UMpadN:
            n = readIntWithMsg( "Dimension: " );
            return rofi::buildUMpad( n );
        case BotType::UMpadNM:
            n = readIntWithMsg( "Dimension n: " );
            m = readIntWithMsg( "Dimension m: " );
            return rofi::buildUMpad( n, m );
        case BotType::NPad:
            n = readIntWithMsg( "Dimension: " );
            rofibot.insert( rofi::buildPad( n ) );
            break;
        case BotType::NMPad:
            n = readIntWithMsg( "Dimension n: " );
            m = readIntWithMsg( "Dimension m: " );
            rofibot.insert( rofi::buildPad( n, m ) );
            break;
        default:
            throw std::runtime_error( "Unknown model" );
    }
    return rofibot;
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

int build( Dim::Cli & cli ) {
    auto configuration = buildWishedRofibot( static_cast< BotType >( *botType ) );

    rofi::connect< rofi::RigidJoint >(
        *botType > 2 ? configuration.getModule( 0 )->connector( 0 ) // No body within the pad
                     : configuration.getModule( 0 )->body( 0 ),     // UM, so we fix its body
        Vector( { 0, 0, 0 } ),
        identity );

    configuration.prepare();
    renderConfiguration( configuration, botTypeToString( static_cast< BotType >( *botType ) ) );

    return 0;
}
