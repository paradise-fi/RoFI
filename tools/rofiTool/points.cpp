#include <atoms/cmdline_utils.hpp>
#include <configuration/rofiworld.hpp>
#include <dimcli/cli.h>
#include <parsing/parsing.hpp>

#include "rendering.hpp"


void points( Dim::Cli & cli );

static auto command = Dim::Cli()
                              .command( "points" )
                              .action( points )
                              .desc( "Interactively preview a set of points "
                                     "defining the rofi world" );
static auto & inputWorldFile = command.opt< std::filesystem::path >( "<input_world_file>" )
                                       .defaultDesc( {} )
                                       .desc( "Input world file ('-' for standard input)" );
static auto & worldFormat = command.opt< rofi::parsing::RofiWorldFormat >( "f format" )
                                    .valueDesc( "world_format" )
                                    .desc( "Format of the world file" )
                                    .choice( rofi::parsing::RofiWorldFormat::Json, "json" )
                                    .choice( rofi::parsing::RofiWorldFormat::Voxel, "voxel" )
                                    .choice( rofi::parsing::RofiWorldFormat::Old, "old" );
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

static auto & showModules = command.opt< bool >( "m modules" ).desc( "Show modules" );

void points( Dim::Cli & cli )
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

    renderPoints( std::move( *world ), "Points of " + inputWorldFile->string(), *showModules );
}
