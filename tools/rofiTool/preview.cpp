#include <fstream>
#include <stdexcept>

#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>

#include "common.hpp"
#include "rendering.hpp"


void preview( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "preview" )
                              .action( preview )
                              .desc( "Interactively preview a rofi world" );
static auto & inputFile = command.opt< std::string >( "<world_file>" )
                                  .desc( "Specify rofi world source file" );

void preview( Dim::Cli & cli )
{
    auto world = parseRofiWorld( *inputFile );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while parsing world", world.assume_error() );
        return;
    }
    if ( world->modules().empty() ) {
        cli.fail( EXIT_FAILURE, "Empty rofi world" );
        return;
    }

    if ( world->referencePoints().empty() ) {
        std::cout << "No reference points found, fixing the world in space\n";
        affixRofiWorld( *world );
    }

    if ( auto valid = world->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Invalid rofi world", valid.assume_error() );
        return;
    }

    renderRofiWorld( *world, *inputFile );
}
