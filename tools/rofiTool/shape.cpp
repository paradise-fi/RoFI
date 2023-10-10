#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <isoreconfig/isomorphic.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>

void shape( Dim::Cli & cli );

static auto command = Dim::Cli()
    .command( "shape" )
    .action( shape )
    .desc( "Compare the shapes of two given configurations" );
static auto & firstInputWorldFile = command.opt< std::filesystem::path >( "<first_input_world_file>" )
    .defaultDesc( {} )
    .desc( "First input world file ('-' for standard input)" );
static auto & firstWorldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "f firstFormat" )
    .valueDesc( "first_world_format" )
    .desc( "Format of the first world file" )
    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );
static auto & secondInputWorldFile = command.opt< std::filesystem::path >( "<second_input_world_file>" )
    .defaultDesc( {} )
    .desc( "Second input world file ('-' for standard input)" );
static auto & secondWorldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "s secondFormat" )
    .valueDesc( "second_world_format" )
    .desc( "Format of the second world file" )
    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );

void shape( Dim::Cli & cli ) 
{
    auto firstWorld = atoms::readInput( *firstInputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *firstWorldFormat );
    } );
    if ( !firstWorld ) {
        cli.fail( EXIT_FAILURE, "Error while parsing first world", firstWorld.assume_error() );
        return;
    }

    auto secondWorld = atoms::readInput( *secondInputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *secondWorldFormat );
    } );
    if ( !secondWorld ) {
        cli.fail( EXIT_FAILURE, "Error while parsing second world", secondWorld.assume_error() );
        return;
    }

    // If either world is empty, they have equal shape iff both of them
    // are empty (sum of the number of their modules is 0).
    if ( firstWorld->modules().empty() || secondWorld->modules().empty() )
        exit( int( firstWorld->modules().size() + secondWorld->modules().size() ) );

    if ( firstWorld->referencePoints().empty() )
        rofi::parsing::fixateRofiWorld( *firstWorld );
    if ( secondWorld->referencePoints().empty() )
        rofi::parsing::fixateRofiWorld( *secondWorld );

    if ( auto valid = firstWorld->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "First rofi world is invalid", valid.assume_error() );
        return;
    }
    if ( auto valid = secondWorld->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Second rofi world is invalid", valid.assume_error() );
        return;
    }

    if ( !rofi::isoreconfig::equalShape( *firstWorld, *secondWorld ) ) {
        exit( EXIT_FAILURE ); // Avoid dimcli error message
    }
}

