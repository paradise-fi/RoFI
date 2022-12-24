#include <atoms/parsing.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>

#include "rendering.hpp"


void points( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "points" )
                              .action( points )
                              .desc( "Interactively preview a set of points "
                                     "defining the rofi world" );
static auto & inputFile = command.opt< std::string >( "<world_file>" )
                                  .desc( "Specify rofi world source file" );
static auto & showModules = command.opt< bool >( "modules" ).desc( "Show modules" );

void points( Dim::Cli & cli )
{
    auto world = atoms::parseRofiWorld( *inputFile );
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
        atoms::fixateRofiWorld( *world );
    }

    if ( auto valid = world->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Invalid rofi world", valid.assume_error() );
        return;
    }

    renderPoints( std::move( *world ), *inputFile, *showModules );
}
