#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>

#include "common.hpp"


void check( Dim::Cli & cli );

static auto command = Dim::Cli().command( "check" ).action( check ).desc(
        "Check a given rofi world" );
static auto & inputFile = command.opt< std::string >( "<world_file>" )
                                  .desc( "Specify rofi world source file" );

void check( Dim::Cli & cli )
{
    auto world = parseRofiWorld( *inputFile );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while parsing world", world.assume_error() );
        return;
    }

    // Empty world is valid
    if ( world->modules().empty() ) {
        return;
    }

    if ( world->referencePoints().empty() ) {
        std::cout << "No reference points found, fixing the world in space\n";
        affixRofiWorld( *world );
    }

    if ( !world->validate() ) {
        exit( EXIT_FAILURE ); // Avoid dimcli error message
    }
}
