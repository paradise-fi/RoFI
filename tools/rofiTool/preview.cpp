#include <fstream>
#include <stdexcept>

#include <atoms/cmdline_utils.hpp>
#include <atoms/parsing.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>

#include "rendering.hpp"


void preview( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "preview" )
                              .action( preview )
                              .desc( "Interactively preview a rofi world" );
static auto & inputFile = command.opt< std::filesystem::path >( "<world_file>" )
                                  .defaultDesc( {} )
                                  .desc( "Source rofi world file ('-' for standard input)" );
static auto & worldFormat = command.opt< atoms::RofiWorldFormat >( "f format" )
                                    .valueDesc( "ROFI_WORLD_FORMAT" )
                                    .desc( "Format of the rofi world file" )
                                    .choice( atoms::RofiWorldFormat::Json, "json" )
                                    .choice( atoms::RofiWorldFormat::Old, "old" );

void preview( Dim::Cli & cli )
{
    auto world = atoms::readInput( *inputFile, atoms::parseRofiWorldJson );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while reading rofi world", world.assume_error() );
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

    renderRofiWorld( *world, "Preview of " + inputFile->string() );
}
