#include <atoms/cmdline_utils.hpp>
#include <atoms/parsing.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>


void check( Dim::Cli & cli );

static auto command = Dim::Cli().command( "check" ).action( check ).desc(
        "Check a given rofi world" );
static auto & inputFile = command.opt< std::filesystem::path >( "<world_file>" )
                                  .defaultDesc( {} )
                                  .desc( "Source rofi world file ('-' for standard input)" );
static auto & worldFormat = command.opt< atoms::RofiWorldFormat >( "f format" )
                                    .valueDesc( "ROFI_WORLD_FORMAT" )
                                    .desc( "Format of the rofi world file" )
                                    .choice( atoms::RofiWorldFormat::Json, "json" )
                                    .choice( atoms::RofiWorldFormat::Old, "old" );


void check( Dim::Cli & cli )
{
    auto world = atoms::readInput( *inputFile, []( std::istream & istr ) {
        return atoms::parseRofiWorld( istr, *worldFormat );
    } );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while reading world", world.assume_error() );
        return;
    }

    // Empty world is valid
    if ( world->modules().empty() ) {
        return;
    }

    if ( world->referencePoints().empty() ) {
        std::cout << "No reference points found, fixing the world in space\n";
        atoms::fixateRofiWorld( *world );
    }

    if ( !world->validate() ) {
        exit( EXIT_FAILURE ); // Avoid dimcli error message
    }
}
