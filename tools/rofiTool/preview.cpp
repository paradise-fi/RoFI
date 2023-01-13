#include <fstream>
#include <stdexcept>

#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>

#include "rendering.hpp"


void preview( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "preview" )
                              .action( preview )
                              .desc( "Interactively preview a rofi world" );
static auto & inputWorldFile = command.opt< std::filesystem::path >( "<input_world_file>" )
                                       .defaultDesc( {} )
                                       .desc( "Input world file ('-' for standard input)" );
static auto & worldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "f format" )
                                    .valueDesc( "world_format" )
                                    .desc( "Format of the world file" )
                                    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );
static auto & sequence = command.opt< bool >( "seq sequence" )
                                 .desc( "Preview an array of worlds (default is a single world)" );
static auto & byOne = command.opt< bool >( "b by-one" )
                              .desc( "Fixate all modules by themselves"
                                     " - no roficom connections will be made"
                                     " (only applicable for voxel format)" )
                              .after( []( auto & cli, auto & opt, auto & ) {
                                  if ( *opt ) {
                                      if ( *worldFormat != rofi::parsing::RofiWorldFormat::Voxel ) {
                                          cli.badUsage(
                                                  "`by-one` is only applicable for voxel format" );
                                          return false;
                                      }
                                  }
                                  return true;
                              } );

void previewSingle( Dim::Cli & cli )
{
    auto world = atoms::readInput( *inputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorld( istr, *worldFormat, *byOne );
    } );
    if ( !world ) {
        cli.fail( EXIT_FAILURE, "Error while reading input", world.assume_error() );
        return;
    }

    if ( world->modules().empty() ) {
        cli.fail( EXIT_FAILURE, "Empty world" );
        return;
    }

    if ( world->referencePoints().empty() ) {
        std::cerr << "No reference points found, fixing the world in space\n";
        rofi::parsing::fixateRofiWorld( *world );
    }

    if ( auto valid = world->validate(); !valid ) {
        cli.fail( EXIT_FAILURE, "Invalid world", valid.assume_error() );
        return;
    }

    renderRofiWorld( *world, "Preview of " + inputWorldFile->string() );
}

void previewSequence( Dim::Cli & cli )
{
    auto worldSeq = atoms::readInput( *inputWorldFile, [ & ]( std::istream & istr ) {
        return rofi::parsing::parseRofiWorldSeq( istr, *worldFormat, *byOne );
    } );
    if ( !worldSeq ) {
        cli.fail( EXIT_FAILURE, "Error while reading input sequence", worldSeq.assume_error() );
        return;
    }

    if ( worldSeq->empty() ) {
        cli.fail( EXIT_FAILURE, "No world in sequence" );
        return;
    }

    for ( size_t i = 0; i < worldSeq->size(); i++ ) {
        if ( ( *worldSeq )[ i ].modules().empty() ) {
            cli.fail( EXIT_FAILURE, "Empty world " + std::to_string( i ) );
            return;
        }
    }

    for ( size_t i = 0; i < worldSeq->size(); i++ ) {
        auto & world = ( *worldSeq )[ i ];
        if ( world.referencePoints().empty() ) {
            std::cerr << "No reference points found, fixing the world " + std::to_string( i )
                                 + " in space\n";
            rofi::parsing::fixateRofiWorld( world );
        }

        if ( auto valid = world.validate(); !valid ) {
            cli.fail( EXIT_FAILURE,
                      "Invalid rofi world " + std::to_string( i ),
                      valid.assume_error() );
            return;
        }
    }

    renderRofiWorldSequence( *worldSeq, "Preview of sequence " + inputWorldFile->string() );
}


void preview( Dim::Cli & cli )
{
    if ( *sequence ) {
        previewSequence( cli );
    } else {
        previewSingle( cli );
    }
}
