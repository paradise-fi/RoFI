#include <optional>

#include <atoms/parsing.hpp>
#include <atoms/unreachable.hpp>
#include <configuration/bots/umpad.hpp>
#include <configuration/pad.hpp>
#include <configuration/universalModule.hpp>
#include <dimcli/cli.h>

#include "rendering.hpp"


void build( Dim::Cli & cli );

static auto command = Dim::Cli().command( "build" ).action( build ).desc(
        "Build and show given rofibot" );
static auto & botType = command.opt< int >( "<rofibot_type>" )
                                .desc( "Specify wished rofibot: "
                                       "0 - UM, 1 - UM n pad, "
                                       "2 - UM n x m pad, 3 - RoFiCoM n pad, "
                                       "4 - RoFiCoM n x m pad" );

enum class BotType {
    UM = 0,
    UMpadN,
    UMpadNM,
    NPad,
    NMPad,
};

std::optional< BotType > toBotType( int b )
{
    switch ( static_cast< BotType >( b ) ) {
        case BotType::UM:
        case BotType::UMpadN:
        case BotType::UMpadNM:
        case BotType::NPad:
        case BotType::NMPad:
            return static_cast< BotType >( b );
    }
    return std::nullopt;
}

rofi::configuration::RofiWorld buildWishedRofibot( BotType botType )
{
    using namespace rofi::configuration;

    constexpr auto readIntWithMsg = []( const std::string & msg ) {
        int n;
        std::cout << msg;
        std::cin >> n;
        return n;
    };

    switch ( botType ) {
        case BotType::UM: {
            RofiWorld world;
            world.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
            return world;
        }
        case BotType::UMpadN: {
            int n = readIntWithMsg( "Dimension: " );
            return buildUMpad( n );
        }
        case BotType::UMpadNM: {
            int n = readIntWithMsg( "Dimension n: " );
            int m = readIntWithMsg( "Dimension m: " );
            return buildUMpad( n, m );
        }
        case BotType::NPad: {
            RofiWorld world;
            int n = readIntWithMsg( "Dimension: " );
            world.insert( Pad( 0, n ) );
            return world;
        }
        case BotType::NMPad: {
            RofiWorld world;
            int n = readIntWithMsg( "Dimension n: " );
            int m = readIntWithMsg( "Dimension m: " );
            world.insert( Pad( 0, n, m ) );
            return world;
        }
    }

    ROFI_UNREACHABLE( "Unknown BotType" );
}

std::string botTypeToString( BotType b )
{
    switch ( b ) {
        case BotType::UM:
            return "an universal module";
        case BotType::UMpadN:
        case BotType::UMpadNM:
            return "a pad of universale modules";
        case BotType::NPad:
        case BotType::NMPad:
            return "a RoFICoM pad";
    }

    ROFI_UNREACHABLE( "Unknown BotType" );
}

void build( Dim::Cli & cli )
{
    using namespace rofi::configuration;

    auto bType = toBotType( *botType );
    if ( !bType ) {
        cli.fail( EXIT_FAILURE, "Invalid value for BotType" );
        return;
    }

    auto world = buildWishedRofibot( *bType );
    if ( world.modules().empty() ) {
        cli.fail( EXIT_FAILURE, "Empty rofi world" );
        return;
    }
    atoms::fixateRofiWorld( world );

    if ( auto valid = world.validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Invalid rofi world", valid.assume_error() );
        return;
    }

    renderRofiWorld( world, "Preview of " + botTypeToString( *bType ) );
}
