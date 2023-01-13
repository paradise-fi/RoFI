#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>


void check( Dim::Cli & cli );

static auto command = Dim::Cli().command( "check" ).action( check ).desc(
        "Check a given rofi world" );
static auto & inputWorldFile = command.opt< std::filesystem::path >( "<input_world_file>" )
                                       .defaultDesc( {} )
                                       .desc( "Input world file ('-' for standard input)" );
static auto & worldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "f format" )
                                    .valueDesc( "world_format" )
                                    .desc( "Format of the world file" )
                                    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );


void check( Dim::Cli & cli )
{
    auto world = atoms::readInput( *inputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *worldFormat, false );
    } );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", world.assume_error() );
        return;
    }

    if ( !world->modules().empty() && world->referencePoints().empty() ) {
        std::cerr << "No reference points found, fixing the world in space\n";
        rofi::parsing::fixateRofiWorld( *world );
    }

    if ( !world->validate() ) {
        exit( EXIT_FAILURE ); // Avoid dimcli error message
    }
}
